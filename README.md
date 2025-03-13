# HYACINTH - A database migration tool that help to keep your database schema.

## Building

```bash
$ git clone https://github.com/saturn-xiv/hyacinth.git ~/workspace/hyacinth
$ cd ~/workspace/hyacinth/docker/
$ ./build.sh
$ cd ..
$ ./docker/start.sh
> cmake --preset=default -DCMAKE_BUILD_TYPE=Release
> cmake --build build
> ./build/hyacinth -h
```
