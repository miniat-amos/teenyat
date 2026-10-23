#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e
pushd . > /dev/null # backup the inital directory

print_usage() {
    echo ""
    echo "Usage: ./build.sh [OPTION]..."
    echo ""
    echo "Options:"
    echo "  <no option> Default release build"
    echo "    debug     Build with debug symbols"
    echo "    verbose   Enable verbose output during build"
    echo "    clean     Remove build directory (must be used alone)"
    echo ""
    echo "Examples:"
    echo "    ./build.sh                  # Release build"
    echo "    ./build.sh debug verbose    # Debug build with verbose output"
    echo "    ./build.sh clean            # Clean the project"
    echo ""
}

# Default build directory, type, and verbosity
BUILD_DIR="build"
BUILD_TYPE="Release"
VERBOSE_FLAG=""
ARGS_FOUND=()

for arg in "$@"; do
    for found_arg in "${ARGS_FOUND[@]}"; do
        if [ "$found_arg" == "$arg" ]; then
            echo -e "\nError: Argument '$arg' provided more than once."
            print_usage
            popd > /dev/null
            exit 1
        fi
    done
    ARGS_FOUND+=("$arg")

    case "$arg" in
        debug)
            if [ "$BUILD_TYPE" == "Release" ]; then
                BUILD_TYPE="Debug"
                echo "Configuring for Debug build..."
            else
                echo -e "\nError: 'debug' mode requested when mode was already set."
                print_usage
                popd > /dev/null
                exit 1
            fi
            ;;
        verbose)
            VERBOSE_FLAG="-- VERBOSE=1"
            echo "Enabling verbose output..."
            ;;
        clean)
            if [ $# != 1 ]; then
                echo -e "\nError: Cannot combine 'clean' with other arguments."
                print_usage
                popd > /dev/null
                exit 1
            fi
            echo "Cleaning build directories..."
            rm -rf build
            popd > /dev/null
            # `source build.sh clean` must not close the caller's terminal.
            if [ "${BASH_SOURCE[0]}" != "$0" ]; then
                return 0
            fi
            exit 0
            ;;
        *)
            echo -e "\nError: Invalid parameter, '$arg'"
            print_usage
            popd > /dev/null
            exit 1
            ;;
    esac
done

# Create the build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating directory: $BUILD_DIR"
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..
cmake --build . --parallel $VERBOSE_FLAG

popd > /dev/null  # restore the initial directory

OS_NAME=$(uname -s)

if [ "$OS_NAME" = "Linux" ]; then
    # Linux
    echo ""
    echo "========================================================================"
    echo "To complete your setup of the TeenyAT and make it easier to use the"
    echo "library in code or use its systems and assembler, consider running the"
    echo "following commands (or add them to your shell startup file, such as "
    echo "your ~/.bashrc):"
    echo "========================================================================"
    echo ""
    echo "# 1) Define the root of your build directory (modify as necessary)"
    echo "export TEENYAT_BUILD_ROOT=$PWD/build"
    echo ""
    echo "# 2) Include directory for header when compiling"
    echo "export CPATH=\$TEENYAT_BUILD_ROOT/out/include:\$CPATH"
    echo ""
    echo "# 3) Binary directory to run tnasm, lcd or edison"
    echo "export PATH=\$TEENYAT_BUILD_ROOT/out/bin:\$PATH"
    echo ""
    echo "# 4) Directory for static library linking (.a)"
    echo "export LIBRARY_PATH=\$TEENYAT_BUILD_ROOT/out/lib:\$LIBRARY_PATH"
    echo ""
    echo "# 5) Directory for running programs using the shared libray (.so)"
    echo "export LD_LIBRARY_PATH=\$TEENYAT_BUILD_ROOT/out/lib:\$LD_LIBRARY_PATH"
    echo ""
elif [ "$OS_NAME" = "Darwin" ]; then
    # macOS
    echo ""
    echo "========================================================================"
    echo "To complete your setup of the TeenyAT and make it easier to use the"
    echo "library in code or use its systems and assembler, consider running the"
    echo "following commands (or add them to your shell startup file, such as "
    echo "your ~/.zshrc or ~/.bash_profile):"
    echo "========================================================================"
    echo ""
    echo "# 1) Define the root of your build directory (modify as necessary)"
    echo "export TEENYAT_BUILD_ROOT=$PWD/build"
    echo ""
    echo "# 2) Include directory for header when compiling"
    echo "export CPATH=\$TEENYAT_BUILD_ROOT/out/include:\$CPATH"
    echo ""
    echo "# 3) Binary directory to run tnasm, lcd, or edison"
    echo "export PATH=\$TEENYAT_BUILD_ROOT/out/bin:\$PATH"
    echo ""
    echo "# 4) Directory for static library linking (.a)"
    echo "export LIBRARY_PATH=\$TEENYAT_BUILD_ROOT/out/lib:\$LIBRARY_PATH"
    echo ""
    echo "# 5) Directory for running programs using the shared libray (.dylib)"
    echo "export DYLD_LIBRARY_PATH=\$TEENYAT_BUILD_ROOT/out/lib:\$DYLD_LIBRARY_PATH"
    echo ""
else
    # Windows... probably shouldn't get here running sh script
    echo ""
    echo "You're using the build shell script on Windows when you should"
    echo "be using the Windows batch script, \"build.bat\"."
    echo ""
fi

set +e