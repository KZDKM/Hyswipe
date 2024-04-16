CXXFLAGS=-shared -fPIC --no-gnu-unique -Wall -g -DWLR_USE_UNSTABLE -std=c++23 -O2
INCLUDES = `pkg-config --cflags pixman-1 libdrm hyprland`
SRC = $(wildcard src/*.cpp)
TARGET = Hyswipe.so

all:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)

clean:
	rm ./$(TARGET)

withhyprpmheaders: export PKG_CONFIG_PATH = $(XDG_DATA_HOME)/hyprpm/headersRoot/share/pkgconfig
withhyprpmheaders: all