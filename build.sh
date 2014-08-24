#!/bin/bash

#################################
#################################
#####                       #####
#####  Escape Build Script  #####
#####                       #####
#################################
#################################

usage() {
    echo "Usage: build.sh <target> <platform>"
    echo "    Targets: release debug"
    echo "    Platforms: win32 win64 darwin"
}

TARGET=$1
PLATFORM=$2

SFML_DIR="../SFML"
SFML_LIB="$SFML_DIR/build/lib/"
SFML_EXTLIB="$SFML_DIR/extlibs/libs-mingw/x64/"
SFML_INC="$SFML_DIR/include/"

DEFAULT_TARGET="release"
DEFAULT_PLATFORM=""

case $(cc -dumpmachine) in
    *-darwin*)
        DEFAULT_PLATFORM="darwin"
        ;;
    *)
        DEFAULT_PLATFORM="win64"
        ;;
esac

if [[ -z "$TARGET" ]]
then
    TARGET="$DEFAULT_TARGET"
fi

if [[ -z "$PLATFORM" ]]
then
    PLATFORM="$DEFAULT_PLATFORM"
fi

###########################################################
### Shared Flags - Applies to all targets and platforms ###
###########################################################

S_CXXFLAGS="-std=c++1y -Wall -DGLM_FORCE_RADIANS -DGLEW_STATIC -DGLFW_INCLUDE_GLCOREARB"
S_LDFLAGS=""
S_LDLIBS="-lyaml-cpp -lpng -lz -lsfml-audio-s -lsfml-system-s -lsndfile -lopenal32"

####################################################
### Target Flags - Flags specific to each target ###
####################################################

T_CXXFLAGS=""
T_LDFLAGS=""
T_LDLIBS=""

case $TARGET in
    release)
        T_CXXFLAGS="-Ofast"
        ;;
    debug)
        T_CXXFLAGS="-g"
        T_LDFLAGS="-g"
        ;;
    profile)
        T_CXXFLAGS="-O2 -pg"
        T_LDFLAGS="-pg"
        ;;
    *)
        echo "Invalid target."
        usage
        exit 1
        ;;
esac

########################################################
### Platform Flags - Flags specific to each platform ###
########################################################

P_CXXFLAGS=""
P_LDFLAGS=""
P_LDLIBS=""

case $PLATFORM in
    win32)
        P_CXXFLAGS="-m32 -DSFML_STATIC -I$SFML_INC"
        P_LDFLAGS="-m32 -static -L$SFML_LIB -L$SFML_EXTLIB"
        P_LDLIBS="-lglfw3 -lglew32 -lopengl32 -lgdi32 -lwinmm"
        ;;
    win64)
        P_CXXFLAGS="-DSFML_STATIC -I$SFML_INC"
        P_LDFLAGS="-static -L$SFML_LIB -L$SFML_EXTLIB"
        P_LDLIBS="-lglfw3 -lglew32 -lopengl32 -lgdi32 -lwinmm"
        ;;
    darwin)
        P_CXXFLAGS="-stdlib=libc++ -I/opt/local/include"
        P_LDFLAGS="-L/opt/local/lib"
        P_LDLIBS="-lglfw -lGLEW -framework OpenGL -framework Cocoa -framework IOkit"
        ;;
    *)
        echo "Invalid platform."
        usage
        exit 2
        ;;
esac

####################################
### Export variables for Respite ###
####################################

export CXX="g++"
export CXXFLAGS="$CXXFLAGS $S_CXXFLAGS $T_CXXFLAGS $P_CXXFLAGS"
export LDFLAGS="$LDFLAGS $S_LDFLAGS $T_LDFLAGS $P_LDFLAGS"
export LDLIBS="$LDLIBS $S_LDLIBS $T_LDLIBS $P_LDLIBS"

echo $CXX $CXXFLAGS $LDFLAGS $LDLIBS

############################################################
### Respite Cache - Used to create multiple build caches ###
############################################################

RESPITE_CACHE=".respite-$TARGET-$PLATFORM"

mkdir -p $RESPITE_CACHE

case $PLATFORM in
    win*)
        cmd //c "mklink /J .respite $RESPITE_CACHE"
        ;;
    *)
        ln -s "$RESPITE_CACHE" .respite
        ;;
esac

######################################
### Check for existing executables ###
######################################

EXE="escape-$TARGET-$PLATFORM"

case $PLATFORM in
    win*)
        EXE="${EXE}.exe"
        ;;
esac

if [[ -f $EXE ]]
then
    mv $EXE a.respite
fi

###################################
### Respite - Build the project ###
###################################

if respite 2>error.log
then
    mv a.respite $EXE
else
    cat error.log
    echo "BUILD FAILED"
fi

case $PLATFORM in
    win*)
        cmd //c "rd .respite"
        ;;
    *)
        rm .respite
        ;;
esac
