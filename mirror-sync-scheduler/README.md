# Sync Scheduler

The Sync Scheduler is in charge of updating the repositories for all of the
projects within [](https://mirror.clarkson.edu/projects). To do this it reads
the `mirrors.json` file and creates a schedule of when each project wants to run
based on the number of times each day each project wants to sync and the least
common multiple of all the task frequencies. The Sync Scheduler then uses this
schedule to add tasks to the queue. The Queue is first in first out and uses
threads to run up to a set number of tasks at the same time.

The multithreaded queue is the best approach because we don’t want every project
syncing at the same time at midnight (pure threaded approach). We also don’t
want each task running one at a time as that could create some long queue times
as the queue backs up.

To sync the projects we have 2 options rsync and script. There are also some
projects that have 2 or 3 stage rsync's. And a few rsync projects use a
password. All of these cases are handled for each project.

## Config `sync-scheduler.json`

- If `dry_run` is set to true it wont run the rsync commands and will instead
  log the sync
