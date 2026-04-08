BUILD_DIR      = build

GRAPHICS      ?= GRAPHICS_API_OPENGL_33

RAYLIB_SRC_DIR = external/raylib-5.5/src
RAYLIB_LIB     = $(BUILD_DIR)/libraylib.a

CFLAGS         = -Wall -Wextra -ggdb -I$(RAYLIB_SRC_DIR)
LDFLAGS        = -lm

codigo: main.c $(RAYLIB_LIB) | $(BUILD_DIR)
	cc $(CFLAGS) -o codigo main.c $(RAYLIB_LIB) $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

raylib: $(RAYLIB_LIB)

$(RAYLIB_LIB): | $(BUILD_DIR)
	$(MAKE) -C $(RAYLIB_SRC_DIR) \
		PLATFORM=PLATFORM_DESKTOP \
		GRAPHICS=$(GRAPHICS) \
		RAYLIB_LIBTYPE=STATIC \
		RAYLIB_RELEASE_PATH=../../../$(BUILD_DIR)

clean:
	$(MAKE) -C $(RAYLIB_SRC_DIR) clean
	rm -fr $(BUILD_DIR)
	rm codigo
