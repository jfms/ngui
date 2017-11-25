Ngui
===============

A GUI typesetting display engine and cross platform GUI application development framework based on NodeJS/OpenGL

Goal: developing GUI applications on this basis can take into account both the simplicity and speed of developing WEB applications, as well as the performance and experience of Native applications.

一个GUI的排版显示引擎和跨平台的GUI应用程序开发框架，基于NodeJS/OpenGL

目标：在此基础上开发GUI应用程序可拥有开发WEB应用般简单与速度同时兼顾Native应用程序的性能与体验


* Only `iOS` and `Android` systems are supported for the time being, this does not include `AndroidTV`, because `TV` applications are very different from mobile applications

* From here, [`Go API Index`](doc/index.md) can go to `API Documents Index`


Build and install ngui-tools
===============

build must dependent xcode / python / nodejs / yasm.

install ngui tools `make install`, It takes a long time.

use ngui tools create project.

first create an empty file directory, then execute in the directory

`ngui init`

export ios xcode project `ngui export ios`


# Simple Examples

This is a simple program to display Hello world on the screen

```jsx
import { GUIApplication, Text } from 'ngui'
new GUIApplication().start(
	<Text>hello world!</Text>
)
```

You can get a more detailed [`Examples`]

# Start Usage

If you've never used Ngui before, you can start from here and build your Ngui program step by step.

## Install ngui-tools

First, you need to install the toolkit provided by `Ngui`

* Install `ngui-tools` using nodejs `npm` 

* Open `Terminal` and execute the following command：

```sh
# shell
$ sudo npm install -g ngui-tools

```
	
* Running `ngui-tools` requires dependency on `nodejs` and `python2.7`

* And now do not support the `windows` system, you need to use it under `mac` or `linux`

## Create new project

Create a new `Ngui` project using the following `shell` command：

```sh
# shell
$ mkdir myproject
$ cd myproject
$ ngui init
```

## Build project

This step compresses and packages the JavaScript code and resource files inside the project,
If this is a new project, you can skip this step and go directly to the next step

```js
# shell
$ ngui build
```

## Export project

This step exports [`Xcode`] or [`Android Studio`] project，because you eventually publish the program that be a `.apk` or `.ipa`

```js
# shell
# export xcode ios project
$ ngui export ios
# export android studio project
$ ngui export android
```

After exporting the project, next you can open it using [`Xcode`] and [`Android Studio`]

## Ngui test http server

`ngui-tools` provides a test http server, each time you change the `js` or `jsx` code, you don't have to reinstall every time.

Execute the following code to start it：

```js
# shell
$ ngui
```

# Downloads

* Examples demo [`Android APK`] Install package

* Project [`Source code`] from `Github`


[`Examples`]: https://github.com/louis-tru/ngui/tree/master/demo
[`Xcode`]: https://developer.apple.com/library/content/documentation/IDEs/Conceptual/AppDistributionGuide/ConfiguringYourApp/ConfiguringYourApp.html
[`Android Studio`]: https://developer.android.com/studio/projects/create-project.html
[`Android APK`]: https://github.com/louis-tru/ngui/releases/download/v0.1.0/examples-release.apk
[`NPM`]: https://www.npmjs.com/package/ngui-tools
[`Source code`]: https://github.com/louis-tru/ngui