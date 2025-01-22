package mirrormap.geoip;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.Thread;
import java.math.BigInteger;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.text.ParseException;

import mirrormap.log.Log;
import org.codehaus.plexus.archiver.tar.TarGZipUnArchiver;
import org.codehaus.plexus.logging.console.ConsoleLoggerManager;
import org.codehaus.plexus.util.FileUtils;

import org.lavajuno.lucidjson.JsonObject;
import org.lavajuno.lucidjson.JsonString;

public class GeoIPUpdater implements Runnable {
    private static final int BUFFER_SIZE = 4096;
    private static final String DATABASE_DESTINATION_DIR = "GeoLite2City";
    private final String DATABASE_URL;
    private final String CHECKSUM_URL;
    private final String DATABASE_FILENAME;
    private final String CHECKSUM_FILENAME;
    private final String MAXMIND_LICENSE_KEY;
    private final Log log;

    public GeoIPUpdater() throws IOException, ParseException {
        log = Log.getInstance();
        JsonObject env = JsonObject.from(
                Files.readString(Path.of("configs/mirror-map-env.json"))
        );
        DATABASE_URL = ((JsonString) env.get("database_url")).value();
        CHECKSUM_URL = ((JsonString) env.get("checksum_url")).value();
        DATABASE_FILENAME = ((JsonString) env.get("database_filename")).value();
        CHECKSUM_FILENAME = ((JsonString) env.get("checksum_filename")).value();
        MAXMIND_LICENSE_KEY = ((JsonString) env.get("maxmind_license_key")).value();
    }


    //used to update the database every 24 hours in a thread
    @Override
    public void run(){
        //get a pointer to the maxmind handler object
        GeoIPDatabase maxmind = GeoIPDatabase.getInstance();
        // Configure database for the first time
        maxmind.configure();
        while(true){
            try{
                //TODO: Change to sleep till 1am (or midnight)
                //sleep for 1 day
                Thread.sleep(86400000);
            }
            catch(InterruptedException e){
                break;
            }
            log.info("Updating GeoIP database...");
            //download the database
            downloadDatabase();
            //configure the database handler for the database file
            maxmind.configure();
            log.info("Done updating GeoIP database.");


        }
    }

    //downloads the maxmind GeoLite2-City database
    private void downloadDatabase(){
        try{
            //download the Database tar.gz file and the checksum file
            downloadFile(DATABASE_URL + MAXMIND_LICENSE_KEY, DATABASE_FILENAME);
            downloadFile(CHECKSUM_URL + MAXMIND_LICENSE_KEY, CHECKSUM_FILENAME);

            //retrieve the checksum from the checksum file
            String checksum = Files.readString(Path.of(CHECKSUM_FILENAME)).split(" ", 2)[0];

            //compute the checksum from the database tar.gz file
            byte[] database = Files.readAllBytes(Path.of(DATABASE_FILENAME));
            byte[] hash = MessageDigest.getInstance("SHA-256").digest(database);
            String calculated_checksum = new BigInteger(1, hash).toString(16);

            //check to make sure that both the downloaded checksum and the calculated checksum are the same
            if(!checksum.equals(calculated_checksum)){
                checksum = checksum.substring(1);
                if(!checksum.equals(calculated_checksum)){ //sometimes the checksum from maxmind has a leading 0
                    log.error("GeoIP database checksum failed. Aborting this update.");
                    return;
                }
            }

            log.info("Extracting GeoIP database...");
            //object used to unzip the database tar.gz file
            final TarGZipUnArchiver ua = new TarGZipUnArchiver();

            //set up loging for TarGZip as its required for it to function
            ConsoleLoggerManager manager = new ConsoleLoggerManager();
            manager.initialize();
            ua.enableLogging(manager.getLoggerForComponent(""));

            //create a new File object for the location where we want to extract to
            File destDir = new File(DATABASE_DESTINATION_DIR);
            //if it doesnt exist create the folder 
            //else empty the folder of its contents
            if(!destDir.exists()){
                destDir.mkdir();
            }
            else{
                FileUtils.cleanDirectory(destDir);
            }
            //extract the file
            ua.setSourceFile(new File(DATABASE_FILENAME));
            ua.setDestDirectory(destDir);
            ua.extract();
            log.info("Done extracting GeoIP database.");
        }
        catch(IOException | GeneralSecurityException e){
            log.error("Failed to update GeoIP database.");
            log.debug(e.toString());
        }
    }

    //downloads a file from a given url over http
    private void downloadFile(String url, String fileName) throws IOException{
        //open a http connection to the given url 
        HttpURLConnection httpConn = (HttpURLConnection) new URL(url).openConnection();
        int responseCode = httpConn.getResponseCode();

        //check to make sure that connection is established successfully
        if(responseCode == HttpURLConnection.HTTP_OK){
            //initialize input stream from the http connection
            InputStream inputStream = httpConn.getInputStream();

            //initialize output stream to the file
            FileOutputStream outputStream = new FileOutputStream(fileName);

            //read the bytes of the file from the http input stream and output them to the file output stream
            int bytesRead = -1;
            byte[] buffer = new byte[BUFFER_SIZE];
            while ((bytesRead = inputStream.read(buffer)) != -1){
                outputStream.write(buffer, 0, bytesRead);
            }

            outputStream.close();
            inputStream.close();
        }
        else{
            log.error("Failed to connect to " + url + " - got response " + responseCode);
            throw new IOException("Failed to connect to " + url + " - got response " + responseCode);
        }
    }
}
