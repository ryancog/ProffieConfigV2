#!/bin/sh

echo "Detecting target platform..."
NUM_PROCS=`nproc --all`

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    BUILD_PLATFORM=linux
elif [[ "$OSTYPE" == "darwin"* ]]; then
    BUILD_PLATFORM=macOS
else
    echo "Unsupported build platform: $OSTYPE"
    exit 1
fi

if [[ "$1" == "--CROSS_LINUX" ]]; then
    CMAKE_FLAGS="-DCROSS_LINUX=ON"
    BUILD_PLATFORM=linux
elif [[ "$1" == "--CROSS_WIN32" ]]; then
    CMAKE_FLAGS="-DCROSS_WIN32=ON"
    BUILD_PLATFORM=win32
fi

echo "Platform: $BUILD_PLATFORM"

build_target() {
    echo "Building target $1..."
    local PREV_DIR=`pwd`
    cd src/$1

    mkdir -p build
    cd build

    if ! cmake -DCMAKE_VERBOSE_MAKEFILE=ON $CMAKE_FLAGS .. > cmake.log 2>&1; then
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

deploy_target() { 
    echo "Deploying target $1..."
    if [[ "$BUILD_PLATFORM" == "win32" ]]; then
        cp src/${1}/build/${1}.exe $EXECUTABLE_DIR/
    else
        cp src/${1}/build/${1} $EXECUTABLE_DIR/
    fi
}

setup_deploy() {
    echo "Setting up for build deploy..."
    mkdir -p build
    mkdir -p build/$BUILD_PLATFORM
    if [[ "$BUILD_PLATFORM" == "macOS" ]]; then
        local BUNDLE_PATH=build/$BUILD_PLATFORM/ProffieConfig.app
        local VERSION=`cat VERSION`
        mkdir -p $BUNDLE_PATH
        mkdir -p $BUNDLE_PATH/Contents

        EXECUTABLE_DIR=$BUNDLE_PATH/Contents/MacOS 
        RESOURCE_DIR=$BUNDLE_PATH/Contents/Resources
        LIB_DIR=$BUNDLE_PATH/Contents/Frameworks

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

    elif [[ "$BUILD_PLATFORM" == "win32" ]]; then
        local DEPLOY_PATH=build/$BUILD_PLATFORM
        EXECUTABLE_DIR=$DEPLOY_PATH/bin
        LIB_DIR=$DEPLOY_PATH/bin
        RESOURCE_DIR=$DEPLOY_PATH/resources
    elif [[ "$BUILD_PLATFORM" == "linux" ]]; then
        local DEPLOY_PATH=build/$BUILD_PLATFORM
        EXECUTABLE_DIR=$DEPLOY_PATH/bin
        LIB_DIR=$DEPLOY_PATH/lib
        RESOURCE_DIR=$DEPLOY_PATH/resources
    fi

    mkdir -p $EXECUTABLE_DIR
    mkdir -p $RESOURCE_DIR
    mkdir -p $LIB_DIR
}

deploy_resources() {
    echo "Deploying resources..."
    if [[ "$BUILD_PLATFORM" == "macOS" ]]; then
        cp resources/icons/icon.icns $RESOURCE_DIR/
    elif [[ "$BUILD_PLATFORM" == "win32" ]]; then
        echo "TODO"
    elif [[ "$BUILD_PLATFORM" == "linux" ]]; then
        echo "TODO"
    fi
}

deploy_libs() {
    echo "Deploying libraries..."
    local LIBS
    if [[ "$BUILD_PLATFORM" == "macOS" ]]; then
        local LIB_BUILD_DIR=3rdparty/wxWidgets/install-macOS/lib
        # terminated with - to prevent issues like baseu_[[unwanted]] being added
        LIBS+="libwx_osx_cocoau_xrc- "
        LIBS+="libwx_osx_cocoau_html- "
        LIBS+="libwx_osx_cocoau_qa- "
        LIBS+="libwx_osx_cocoau_core- "
        LIBS+="libwx_baseu_xml- "
        LIBS+="libwx_baseu_net- "
        LIBS+="libwx_baseu- "

    elif [[ "$BUILD_PLATFORM" == "win32" ]]; then
        local LIB_BUILD_DIR="3rdparty/wxWidgets/install-win32/bin"
        cp /opt/mxe/usr/x86_64-w64-mingw32.shared/bin/libgcc_s_seh-1.dll $LIB_DIR/
        cp /opt/mxe/usr/x86_64-w64-mingw32.shared/bin/libwinpthread-1.dll $LIB_DIR/
        cp /opt/mxe/usr/x86_64-w64-mingw32.shared/bin/libstdc++-6.dll $LIB_DIR/
        LIBS+="wxbase330u_gcc_suse.dll "
        LIBS+="wxgtk3330u_core_gcc_suse.dll "

   elif [[ "$BUILD_PLATFORM" == "linux" ]]; then
        local LIB_BUILD_DIR=3rdparty/wxWidgets/install-linux/lib64
        LIBS+="libwx_gtk3u_xrc- "
        LIBS+="libwx_gtk3u_html- "
        LIBS+="libwx_gtk3u_qa- "
        LIBS+="libwx_gtk3u_core- "
        LIBS+="libwx_baseu_xml- "
        LIBS+="libwx_baseu_net- "
        LIBS+="libwx_baseu- "
    fi

    for lib in ${LIBS}; do
        local ORIGINAL=""
        for file in `ls $LIB_BUILD_DIR | grep $lib`; do
            if [[ "$ORIGINAL" == "" ]]; then
                cp $LIB_BUILD_DIR/$file $LIB_DIR/
                strip --strip-all $LIB_DIR/$file
                ORIGINAL=$file
            else
                local PREV_DIR=`pwd`
                cd $LIB_DIR
                ln -s "./$ORIGINAL" "./$file"
                cd $PREV_DIR
            fi
        done
    done
}

build_target "launcher"

setup_deploy
deploy_resources
deploy_libs

deploy_target "launcher"

echo "Done."

