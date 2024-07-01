#!/bin/sh

ORIGINAL_DIR=`pwd`

echo "Detecting platform..."
if [ "$OSTYPE" == "linux-gnu" ]; then
    BUILD_PLATFORM=linux
    TARGET_PLATFORM=linux
elif [[ "$OSTYPE" == "darwin"* ]]; then
    BUILD_PLATFORM=macOS
    TARGET_PLATFORM=macOS
else
    echo "Unsupported build platform: $OSTYPE"
    exit 1
fi
echo "Platform: ${BUILD_PLATFORM}"

echo "Getting target platform..."
if [ "$1" == "--CROSS_LINUX" ]; then
    TARGET_PLATFORM=linux
elif [ "$1" == "--CROSS_WIN32" ]; then
    TARGET_PLATFORM=win32
fi

if ! command -v git &> /dev/null; then
    echo "Err: git is missing, please install it!"
    exit 1
fi
echo "Target Platform: ${TARGET_PLATFORM}"

echo "Initializing wxWidgets Repository..."
git submodule update --init
cd 3rdparty/wxWidgets
git submodule update --init
echo "wxWidgets repo initialized."

WX_BUILD_DIR=build-$TARGET_PLATFORM
echo "Preparing wxWidgets for ${TARGET_PLATFORM} build in ${WX_BUILD_DIR}..."
mkdir -p $WX_BUILD_DIR
cd $WX_BUILD_DIR

WX_INSTALL_PREFIX=`pwd`/../install-$TARGET_PLATFORM
WX_FLAGS='--enable-shared --disable-unsafe-conv-in-wxstring'
if [ "$TARGET_PLATFORM" == "linux" ]; then
    WX_HOST='x86_64-linux'
    WX_PLATFORM_FLAGS='--with-gtk=3'
elif [ "$TARGET_PLATFORM" == "macOS" ]; then
    WX_HOST=''
    WX_PLATFORM_PLATS='--with-osx -LDFLAGS="-Wl,-rpath,@loader_path/lib"'
elif [ "$TARGET_PLATFORM" == "win32" ]; then
    WX_HOST='x86_64-w64-mingw32.static'
    WX_PLATFORM_FLAGS='--with-gtk=3'
fi

if [ "$BUILD_PLATFORM" == "linux" ]; then
    WX_BUILD='x86_64-linux'
elif [ "$BUILD_PLATFORM" == "macOS" ]; then
    WX_BUILD=''
fi

echo "Running wxWidgets configure script..."
if ! ../configure --prefix=$WX_INSTALL_PREFIX --host=$WX_HOST --build=$WX_BUILD $WX_FLAGS $WX_PLATFORM_FLAGS &> $ORIGINAL_DIR/wxconfigure.log; then
    echo "Error configuring wxWidgets! Check wxconfigure.log."
    exit 1
fi
echo "Building wxWidgets..."
if ! make -j`nproc --all` &> $ORIGINAL_DIR/wxbuild.log; then
    echo "Error building wxWidgets! Check wxbuild.log."
    exit 1
fi
echo "Installing wxWidgets to $WX_INSTALL_PREFIX..."
if ! make install &> $ORIGINAL_DIR/wxinstall.log; then
    echo "Error installing wxWidgets! Check wxinstall.log."
    exit 1
fi


if [[ "$OSTYPE" == "darwin"* ]]; then
    # set install_name of libs correctly
    # To be completely honest, I don't fully understand this.
    # It comes from an old, tweaked version of wxWidgets' 
    # change_names_tool... found it on an old forum post.
    # "using @rpath on the mac"
    #
    # Basically it's just changing the link names of all the libs to @rpath/[name] that way
    # the install location can be dynamically changed when building the actual application(s).
    echo "Patching wxWidgets libs with @rpath..."
    cd $WX_INSTALL_PREFIX
    WX_LIB_DIR=$WX_INSTALL_PREFIX/lib
    WX_LIB_NAME_PREFIX=@rpath

    WX_LIBNAMES=`cd ${WX_LIB_DIR}; ls -1 | grep '\.[0-9][0-9]*\.dylib$'`
    for i in ${WX_LIBNAMES}; do
        install_name_tool -id ${WX_LIB_NAME_PREFIX}/${i} ${WX_LIB_DIR}/${i}
        for dep in ${WX_LIBNAMES}; do
            install_name_tool -change ${WX_LIB_DIR}/${dep} ${WX_LIB_NAME_PREFIX}/${dep} ${WX_LIB_DIR}/${i}
        done
    done
fi

cd $ORIGINAL_DIR

echo "Cleaning up..."
# Success, logs are not needed!
rm -rf wxconfigure.log
rm -rf wxbuild.log
rm -rf wxinstall.log


echo "Done!"

