#!/bin/sh

NUM_PROCS=`nproc --all`
CLEAN=false

echo "Detecting target platform..."
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
fi
echo "Platform: $BUILD_PLATFORM"

build_target() {
    echo "Building target $1..."
    local PREV_DIR=`pwd`
    cd src/$1

    if $CLEAN; then
        rm -rf build*
        return
    fi

    local BUILD_DIR=build-$BUILD_PLATFORM
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    if ! cmake $CMAKE_FLAGS .. > cmake.log 2>&1; then
        echo "Error running CMake! See the log in the build directory."
        exit 1;
    fi
    if ! make -j$NUM_PROCS > make.log 2>&1; then
        echo "Error building!! See the log in the build directory."
        exit 1;
    fi

    # Success, logs are not needed.
    rm -rf cmake.log
    rm -rf make.log
    cd $PREV_DIR
}

build_target "launcher"
if $CLEAN; then
    exit 0;
fi

mkdir -p build
mkdir -p build/$BUILD_PLATFORM
deploy_target() { cp src/$1/build-$BUILD_PLATFORM/$1 $EXECUTABLE_DIR/
}
setup_deploy() {
    echo "Setting up for build deploy..."
    if [ "$BUILD_PLATFORM" == "macOS" ]; then
        local BUNDLE_PATH=build/$BUILD_PLATFORM/ProffieConfig.app
        local VERSION=`cat VERSION`
        mkdir -p $BUNDLE_PATH
        mkdir -p $BUNDLE_PATH/Contents

        EXECUTABLE_DIR=$BUNDLE_PATH/Contents/MacOS 
        RESOURCE_DIR=$BUNDLE_PATH/Contents/Resources
        LIB_DIR=$BUNDLE_PATH/Contents/Frameworks
        mkdir -p $EXECUTABLE_DIR
        mkdir -p $RESOURCE_DIR
        mkdir -p $LIB_DIR

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

    elif [ "$BUILD_PLATFORM" == "win32" ]; then
        echo ""
    elif [ "$BUILD_PLATFORM" == "linux" ]; then
        echo ""
    fi
}
deploy_resources() {
    echo "Deploying resources..."
    if [ "$BUILD_PLATFORM" == "macOS" ]; then
        cp resources/icons/icon.icns $RESOURCE_DIR/

    elif [ "$BUILD_PLATFORM" == "win32" ]; then
        echo ""
    elif [ "$BUILD_PLATFORM" == "linux" ]; then
        echo ""
    fi
}
deploy_libs() {
    echo "Deploying libraries..."
    if [ "$BUILD_PLATFORM" == "macOS" ]; then
        local LIBS
        # terminated with - to prevent issues like baseu_[unwanted] being added
        LIBS+="libwx_osx_cocoau_xrc- "
        LIBS+="libwx_osx_cocoau_html- "
        LIBS+="libwx_osx_cocoau_qa- "
        LIBS+="libwx_osx_cocoau_core- "
        LIBS+="libwx_baseu_xml- "
        LIBS+="libwx_baseu_net- "
        LIBS+="libwx_baseu- "

        for lib in ${LIBS}; do
            local ORIGINAL=""
            for file in `ls 3rdparty/wxWidgets/install-macOS/lib | grep $lib`; do
                if [ "$ORIGINAL" == "" ]; then
                    cp 3rdparty/wxWidgets/install-macOS/lib/$file $LIB_DIR/
                    ORIGINAL=$file
                else
                    local PREV_DIR=`pwd`
                    cd $LIB_DIR
                    ln -s "./$ORIGINAL" "./$file"
                    cd $PREV_DIR
                fi
            done
        done

    elif [ "$BUILD_PLATFORM" == "win32" ]; then
        echo ""
    elif [ "$BUILD_PLATFORM" == "linux" ]; then
        echo ""
    fi
}

setup_deploy
deploy_resources
deploy_libs
deploy_target "launcher"
echo "Done."

