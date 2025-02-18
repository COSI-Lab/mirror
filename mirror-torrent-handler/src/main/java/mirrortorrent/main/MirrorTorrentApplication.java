package mirrortorrent.main;

import java.io.File;
import java.io.FileNotFoundException;
import java.text.ParseException;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.LocalTime;
import java.time.temporal.ChronoUnit;

import org.lavajuno.lucidjson.JsonArray;
import org.lavajuno.lucidjson.JsonNumber;
import org.lavajuno.lucidjson.JsonObject;
import org.lavajuno.lucidjson.JsonString;

import mirrortorrent.io.Log;
import mirrortorrent.torrents.ScrapeTorrents;
import mirrortorrent.torrents.SyncTorrents;

public class MirrorTorrentApplication {

    public static void main(String[] args) {
        final String envFilePath = "configs/torrent-handler.json";
        String torrentFolder = "";
        String downloadFolder = "";
        String logServerHost = "";
        int logServerPort = -1;

        try {
            // Read config into a JSON object
            JsonObject env = JsonObject.fromFile(envFilePath);
            torrentFolder = ((JsonString) env.get("torrentFolder")).getValue();
            downloadFolder = ((JsonString) env.get("downloadFolder")).getValue();
            logServerHost = ((JsonString) env.get("logServerHost")).getValue();
            logServerPort = ((JsonNumber) env.get("logServerPort")).getInt();
        } catch (FileNotFoundException e) {
            System.out.println("FATAL: Failed to open environment file " + envFilePath);
            System.exit(1);
        } catch (ParseException e) {
            System.out.println("FATAL: Failed to parse environment file " + envFilePath);
            System.exit(1);
        }

        // Initialize Logger
        Log log = Log.getInstance();
        if (!logServerHost.equals("") && logServerPort != -1) {
            log.configure(logServerHost, logServerPort, "Torrent Handler");
        } else {
            System.out.println("Log Server Host or Port not set! Exiting.");
            System.exit(1);
        }

        // If torrents directory doesn't exist, create it
        if (!torrentFolder.equals("")) {
            File torrentsDirectory = new File(torrentFolder);
            if (!torrentsDirectory.exists()) {
                log.info("Creating torrents directory");
                torrentsDirectory.mkdir();
            }
        } else {
            log.fatal("Torrents directory not set! Exiting.");
            System.exit(1);
        }

        // If downloads directory doesn't exist, create it.
        if (!downloadFolder.equals("")) {
            File downloadsDirectory = new File(downloadFolder);
            if (!downloadsDirectory.exists()) {
                log.info("Creating downloads directory");
                downloadsDirectory.mkdir();
            }
        } else {
            log.fatal("Downloads directory not set! Exiting.");
            System.exit(1);
        }

        try {
            // Load config from mirrors.json
            JsonObject config = JsonObject.fromFile("configs/mirrors.json");

            // Spawn Torrent Scraper Thread
            Thread torrentScrapeThread = new Thread(
                    new ScrapeTorrents((JsonArray) config.get("torrents"), torrentFolder));
            torrentScrapeThread.start();
            log.info("Scrape Torrents Thread Started");

            // Spawn Torrent Syncing Thread
            Thread syncTorrentsThread = new Thread(
                    new SyncTorrents((JsonObject) config.get("mirrors"), torrentFolder, downloadFolder));
            syncTorrentsThread.start();
            log.info("Sync Torrents Thread Started");

            // Join Threads Upon Completion
            syncTorrentsThread.join();
            log.info("Joined Sync Torrents Thread");
            torrentScrapeThread.join();
            log.info("Joined Torrent Scraper Thread");
        } catch (FileNotFoundException | ParseException | InterruptedException e) {
            log.fatal(e.getMessage());
        }

        // Sleep Until 1am The Following Day
        LocalDateTime currentTime = LocalDateTime.now();
        LocalDate targetDate = LocalDate.now().plusDays(1);
        LocalTime targetTime = LocalTime.of(1, 0);
        LocalDateTime timeToWake = LocalDateTime.of(targetDate, targetTime);
        long millisecondsToSleep = ChronoUnit.MILLIS.between(currentTime, timeToWake);

        try {
            Thread.sleep(millisecondsToSleep);
        } catch (InterruptedException e) {
            log.fatal(e.getMessage());
        }
    }
}
