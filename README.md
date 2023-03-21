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
$ catcare get <user>/<repo>@<branch>
```
This command will add a `catpkgs` directory and install the project with all it's dependencies in there.

**Note:** this is the default layout that downloads from github, it can be changed using the [urlrules.ccr]()!

## Browse through projects
Not sure what to install? You can use the `browse` option to browse through official or community lists of projects!  
Simply
```
$ catcare browse official
```
or
```
$ catcare browse <repo>
```
and enjoy the read!

## Removing a project
```
$ catcare remove <project>
```
..to remove on project and
```
$ catcare remove .all
```
..to remove all projects installed.

## config directory
The config directoy is where the catcaretaker will store global data.  
Linux: `~/.config/catcaretaker/`  
Windows: `C:\ProgramData\.catcaretaker` 

File tree:
```
catcaretaker/
| - attachments/
| - macros/
| - extensions/
| - urlrules.ccr
| - config.inipp
``` 
`attachments/` see [urlrules.ccr]()  
`macros/` see [macros]()  
`extensions/` see [carescript]()  
`urlrules.ccr` see [urlrules.ccr]()  

`config.inipp` stores all your set settings, you can see the available ones with
```
$ catcare config explain
```
and set them with
```
$ catcare option <name> to <value>
```

## urlrules.ccr
This file is located in the [config directory]() and contains information on how to turn the user input like
`username/repo@branch` into an URL to download from. It uses it's own markdown language.  
You define different `rules`, where each `rule` defines a `link` and some `placeholders`.  
Syntax example:
```
RULE github;
  "defines the link and 3 placeholders"
  LINK "https://raw.githubusercontent.com/{user}/{project}/{branch}/"
  "defines which token in the user input is which placeholder"
  WITH user 1
       project 2
       branch 3;
  "defines which tokens are used to split the user input, if too less, last one gets repeated"
  SIGNS / @;
  "you can set defaults for specific placeholders"
  DEFAULT branch "master"
  "optional: attach a catcarescript when downloading from this rule"
  ATTACH "script.ccs";
  "execute the scripts before the download"
  SCRIPTS BEFORE; 
  "embed carescript code right into this file"
  EMBED AFTER {
    echoln("Hello, World!"
  }
```
This rule is now able to turn a string like `name/proj@main` into   
`https://raw.githubusercontent.com/name/proj/main/`, which then gets downloaded by the catcaretaker.  
Script files attached with the `ATTACH` keyword bust be placed into the `attachment` config directory.

## carescript
Carescript is the embedded scripting language the catcaretaker uses. It's made by me, lol, and can be found [here]().  
It comes with an always loaded "catcaretaker" extension, to add fundamental functionality, and further extensions can be loaded by  
placing the file into the `extensions` config directory.  
An example on how to use the catcaretaker extension:
```py
@main[]
  # downloads a file from an url
  download(url, destination)
  
  add_file(path, content)
  add_directory(path)
  
  # checks if a file or directory exists
  exists(path)
  
  remove(path)
  copy(path, destination)
  move(path, destination)
  
  # checks if a dependency is installed (by name or url)
  installed(name)
  
  # adds a dependency
  add_project(url)
  
  # translate the input using the urlrules.ccr
  resolve(input)
  
  # downloads a project's files into the destination without adding it
  # to the dependencylist, aka "dumb download" 
  download_project(url, destination)
  
  # from the global config file
  get_config(name)
  set_config(name, value)
  
  # gets information from a cat_checklist.inipp
  checklist(path, key)
  
  # and some macros:
  # ROOT -> path to the catpkgs directory
  # MACRO_DIR -> path to the macro directory
  # EXTENSION_DIR -> ...
  # HOME -> path to the config directory
  # CONFIG -> path to the condig file
  # CHECKLIST -> the name of the cat_checklist.inipp
  # PROGNAME -> "catcaretaker"
```

## macros
Macros are user defined scripts that can be executed by running
```
$ catcare macro <name> <args...>
```
These macro files are in the macro config directory, see [this paragraph].  

The basic structure for a macro looks like this:
```
@macro_call[args...]
  # this will get executed in the same path as
  # the user executing the call
```
The catcaretaker carescript extension as well as the extensions in the extension directory  
are also all loaded.  
Macros are intended to be little tools to help the user with their packages, but of course
they can be whatever you want them to be!
