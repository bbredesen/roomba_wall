CSOURCES := $(wildcard *.c)
CPPSOURCES := $(wildcard *.cpp)
BUILDDIR := build
OBJECTS := $(addprefix $(BUILDDIR)/, $(CSOURCES:.c=.o))
# $(CPPSOURCES:.cpp=.o)

OUTPUT := $(BUILDDIR)/roomba_wall
HEXFILE := $(OUTPUT).hex

all: compile
	avrdude -c usbtiny -p t85 -v -U flash:w:$(HEXFILE):i

compile: $(BUILDDIR) $(OBJECTS)
	avr-gcc -Wall -Os -DF_CPU=1000000 -mmcu=attiny85 -o $(OUTPUT) $(OBJECTS)
	avr-objcopy -j .text -j .data -O ihex $(OUTPUT) $(HEXFILE)

$(BUILDDIR)/%.o: %.c
	avr-gcc -Wall -Os -DF_CPU=1000000 -mmcu=attiny85 -c -o $@ $<

%.o: %.cpp
	avr-gcc -Wall -Os -DF_CPU=1000000 -mmcu=attiny85 -c -o $@ $<

%.c: %.h

$(BUILDDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR)
	#/*.o $(HEXFILE) $(OUTPUT)
