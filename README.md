# mirror

Software for Clarkson University's open source mirror.

# Building

To build mirror, make sure you have `docker` and `docker-compose-v2` installed.
Each module uses builder containers with the proper tools, so
you don't need to install anything extra.

Once you have docker installed, `git clone` this repository somewhere on your machine.
At the repository root, run `make dev`. This will use the development docker compose
configuration and run each module with the configs located in the module's directory.

To emulate a production environment, make sure the directories `/storage`
and `/var/log/nginx` exist. Also, make sure configs exist in the `configs` directory
at the repository root. Run `make prod` to use the production docker compose configuration
and run each module as if it was running on Mirror.

We currently use the development configuration on our personal machines and
the production configuration on a test server. Both are frequently tested.

# Contributing

If you would like to contribute to Mirror, create a branch and open a pull request
with your changes. You can also create issues if you think something can be improved
to open up opportunities to others to make contributions too. If you're not sure
where to start, documentation is always a great place.


> # Aside: Project Goals
> 
> ## Modularity
> A modular Mirror allows us to write simple components that
> [Do One Thing and Do it Well.](https://en.wikipedia.org/wiki/Unix_philosophy)
> This eases the learning curve for younger students by allowing them to work
> in a familiar environment on small programs, while keeping the opportunity to
> learn about interactions between modules in a large software project open.
>
> Each module of Mirror should be assigned an easily describable purpose,
> for example:
>
> - The sync scheduler keeps our mirrors synced with upstream mirrors.
>
> - The website serves pages to visitors of mirror.clarkson.edu
>
> - The API serves information about what we mirror to other modules.
>
> - The log server processes log messages from other modules.
>
> - The metrics engine collects data about how Mirror is used.
>
> ## Maintainability
>
> Mirror should be easy to maintain for future students. To make this possible,
> it should be documented, structured, and written in in a way that students who
> may have not yet taken higher-level CS classes can learn and contribute to.
>
> ## Community
>
> Contributing to Mirror's source code, documentation, and maintenance should be
> an experience that many students at COSI take part in. Knowledge about Mirror
> should be available both as documentation and as in-person help for 
> contributors.
>
> ## Free and Open Source Software
> 
> While Mirror's primary purpose is to run our open source mirror, it should be
> available to others who wish to mirror free and open source software. This 
> goes beyond just an open source license, Mirror should be designed with
> adaptability to different environments in mind.
>
