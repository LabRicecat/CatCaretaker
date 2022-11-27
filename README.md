# CatCaretaker
A tool to easily reuse projects

## Installation
Run these commands to download and install the CatCaretaker
```
$ git clone https://github.com/LabRiceCat/catcaretaker
$ cd catcaretaker
$ mkdir build
$ cd build
$ cmake ..
$ make
```

After that, easily run it with:
```
$ catcare --help
```
Now it's a good time to run the config helper with:
```
$ catcare guide
```

## Seting up a project
To set up a project run this commands:
```
$ catcare setup <projectname>
```
This will create a `cat_checklist.inipp` file which stores data about the current project.

## Installing a project
To install a project and add it to your dependencies run:
```
$ catcare get <user>/<repo>
```
If `<user>/` is not specified, the default user will be used.

This command will add a `catmods` directory and install the project with all it's dependencies in there.

## Removing a project
```
$ catcare erase <project>
```
To remove on project and
```
$ catcare cleanup
```
..to remove all projects installed.

## Using a local project
If you want to use a project that is locally installed, use the following command:
```
$ catcare local <path>
```
But you shouldn't use `<path>`, the better way is to make a global redirector/alias for it.

```
$ catcare redirect <name> to <path>
```
This way when we use `<name>` it will be replaced with `<path>`.
