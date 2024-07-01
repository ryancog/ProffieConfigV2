#!/bin/sh

NUM_PROCS=`nproc --all`
CLEAN=false

if [ "$OSTYPE" == "linux-gnu" ]; then
    BUILD_PLATFORM=linux
elif [[ "$OSTYPE" == "darwin"* ]]; then
    BUILD_PLATFORM=macOS
else
    echo "Unsupported build platform: $OSTYPE"
    exit 1
fi
if [ "$1" == "--CROSS_LINUX" ]; then
    CMAKE_FLAGS="-DCROSS_LINUX=ON"
    BUILD_PLATFORM=linux
elif [ "$1" == "--CROSS_WIN32" ]; then
    CMAKE_FLAGS="-DCROSS_WIN32=ON"
    BUILD_PLATFORM=win32
elif [ "$1" == "--CLEAN" ]; then
    CLEAN=true
    echo "Cleaning everything!!"
fi

build_target() {
    local PREV_DIR=`pwd`
    cd src/$1

    if $CLEAN; then
        rm -rf build*
        return
    fi

    local BUILD_DIR=build-$BUILD_PLATFORM
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    if ! cmake $CMAKE_FLAGS ..; then
        echo "Error running CMake!"
        exit 1;
    fi
    if ! make -j$NUM_PROCS; then
        echo "Error building!!"
        exit 1;
    fi

    cd $PREV_DIR
}

build_target "launcher"
if $CLEAN; then
    exit 0;
fi

mkdir -p build
mkdir -p build/$BUILD_PLATFORM
deploy_target() {
    if [ "$BUILD_PLATFORM" == "macOS" ]; then
        local BUNDLE_PATH=build/$BUILD_PLATFORM/ProffieConfig.app
        cp src/$1/build-$BUILD_PLATFORM/$1 $BUNDLE_PATH/Contents/MacOS/
    elif [ "$BUILD_PLATFORM" == "win32" ]; then
        echo ""
    elif [ "$BUILD_PLATFORM" == "linux" ]; then
        echo ""
        # local TARGET_DIR=build/${BUILD_PLATFORM}/${1}
        # mkdir -p $TARGET_DIR

        # cp src/${1}/build-${BUILD_PLATFORM}/${1}* $TARGET_DIR/
    fi
}
setup_deploy() {
    if [ "$BUILD_PLATFORM" == "macOS" ]; then
        local BUNDLE_PATH=build/$BUILD_PLATFORM/ProffieConfig.app
        local VERSION=`cat VERSION`
        mkdir -p $BUNDLE_PATH
        mkdir -p $BUNDLE_PATH/Contents
        mkdir -p $BUNDLE_PATH/Contents/MacOS
        mkdir -p $BUNDLE_PATH/Contents/Resources
        mkdir -p $BUNDLE_PATH/Contents/Frameworks
        printf "
            <?xml version=\"1.0\" encoding=\"UTF-8\"?>
            <!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
            <plist version=\"1.0\">
            <dict>
                <key>CFBundleDevelopmentRegion</key>
                <string>en</string>
                <key>CFBundleExecutable</key>
                <string>launcher</string>
                <key>CFBundleIconFile</key>
                <string>icon.icns</string>
                <key>CFBundleIdentifier</key>
                <string>com.kafrenetrading.proffieconfig</string>
                <key>CFBundlePackageType</key>
                <string>APPL</string>
                <key>CFBundleShortVersionString</key>
                <string>${VERSION}</string>
                <key>LSMinimumSystemVersion</key>
                <string>10.14</string>
                <key>NSPrincipleClass</key>
                <string>NSApplication</string>
                <key>NSSupportsAutomaticGraphicsSwitching</key>
                <true/>
            </dict>
            </plist>
         " > $BUNDLE_PATH/Contents/Info.plist
         cp resources/icons/icon.icns $BUNDLE_PATH/Contents/Resources/
    elif [ "$BUILD_PLATFORM" == "win32" ]; then
        echo ""
    elif [ "$BUILD_PLATFORM" == "linux" ]; then
        echo ""
    fi
}
setup_deploy
deploy_target "launcher"

