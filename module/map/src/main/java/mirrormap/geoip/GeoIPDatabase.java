package mirrormap.geoip;

import java.io.File;
import java.io.IOException;
import java.net.InetAddress;

import com.maxmind.geoip2.DatabaseReader;
import com.maxmind.geoip2.exception.GeoIp2Exception;
import com.maxmind.geoip2.model.CityResponse;
import mirrormap.log.Log;

public class GeoIPDatabase {

    private static final String DATABASE_DESTINATION_DIR = "GeoLite2City";

    private final Log log;

    //used to read data from the maxmind database
    private static DatabaseReader reader = null;

    //used to make this class a singleton class
    private static GeoIPDatabase instance = null;

    //private default constructor for singleton
    private GeoIPDatabase() {
        log = Log.getInstance();
    }

    //get instance function for accessing the singleton class
    public static synchronized GeoIPDatabase getInstance(){
        if(instance == null){ instance = new GeoIPDatabase(); }
        return instance;
    }

    //initialize the maxmind database handler based on its file location
    public void configure(){
        log.info("Configuring GeoIP database...");
        try{
            File geoLite2Directory = new File(DATABASE_DESTINATION_DIR + "/" + new File(DATABASE_DESTINATION_DIR).list()[0] + "/GeoLite2-City.mmdb");
            reader = new DatabaseReader.Builder(geoLite2Directory).build();
        }
        catch(IOException e){
            log.error("Failed to configure GeoIP database.");
            log.debug(e.toString());
        }
        log.info("Done configuring GeoIP database.");
    }

    //used to get the latitude and longitude based on the ip using the maxmind database reader
    public double[] getLatLong(String ipAddress) throws IOException, GeoIp2Exception {
        if(reader == null) {
            throw new GeoIp2Exception("Unable to get location - GeoIP database not configured.");
        }
        InetAddress ip = InetAddress.getByName(ipAddress);
        CityResponse response = reader.city(ip);
        Double latitude = response.getLocation().getLatitude();
        Double longitude = response.getLocation().getLongitude();
        if(latitude == null | longitude == null) {
            throw new IOException("Failed to get latitude/longitude from GeoIP database.");
        }
        return new double[]{latitude, longitude};
    }

}
