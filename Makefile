BUILD_DIR=build
T3D_INST=~/Devel/tiny3d

LIBSPZ=$(BUILD_DIR)/libspz.a

include $(N64_INST)/include/n64.mk
include $(T3D_INST)/t3d.mk

N64_CFLAGS += -std=gnu2x -O2
GLTF_FLAGS = ''

PROJECT_NAME=t3d_18_particles

CFLAGS += -Iinclude -Ispz/src/cc/ -Wno-error
CXXFLAGS += -Iinclude -Ispz/src/cc/ -Itinyzzz -Wno-error

src = $(wildcard src/*.c)
assets_png = $(wildcard assets/*.png)
assets_gltf = $(wildcard assets/*.glb)
assets_ttf = $(wildcard assets/*.ttf)
assets_splat = $(wildcard assets/*.ply)

assets_conv = $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
			  $(addprefix filesystem/,$(notdir $(assets_ttf:%.ttf=%.font64))) \
			  $(addprefix filesystem/,$(notdir $(assets_gltf:%.glb=%.t3dm))) \
			  $(addprefix filesystem/,$(notdir $(assets_splat:%.ply=%.splat)))

filesystem/scene.t3dm: GLTF_FLAGS = --bvh
filesystem/platformer.t3dm: GLTF_FLAGS = --bvh

all: $(PROJECT_NAME).z64

$(BUILD_DIR)/spz/%.o: spz/src/cc/%.cc | $(BUILD_DIR)/spz/
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: tinyzzz/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

OBJ := $(src:%.c=$(BUILD_DIR)/%.o)

SPZ_OBJS := $(BUILD_DIR)/spz/load-spz.o \
            $(BUILD_DIR)/spz/splat-c-types.o \
            $(BUILD_DIR)/spz/splat-types.o 

ZSTD_OBJ := $(BUILD_DIR)/zstdD.o

$(LIBSPZ): $(SPZ_OBJS) | $(BUILD_DIR)/spz/
	$(AR) rcs $@ $^

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

filesystem/%.font64: assets/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	$(N64_MKFONT) $(MKFONT_FLAGS) -s 9 -o filesystem "$<"

filesystem/%.t3dm: assets/%.glb
	@mkdir -p $(dir $@)
	@echo "    [T3D-MODEL] $@"
	$(T3D_GLTF_TO_3D) $(GLTF_FLAGS) "$<" $@
	$(N64_BINDIR)/mkasset -c 2 -o filesystem $@

filesystem/%.splat: assets/%.spz
	@mkdir -p $(dir $@)
	@echo "    [SPLAT] $@"
	./tools/spz2tiny3d $< $@

$(BUILD_DIR)/$(PROJECT_NAME).dfs: $(assets_conv)
$(BUILD_DIR)/$(PROJECT_NAME).elf: $(OBJ) $(ZSTD_OBJ)

$(PROJECT_NAME).z64: N64_ROM_TITLE="TinyPX - Particles"
$(PROJECT_NAME).z64: $(BUILD_DIR)/$(PROJECT_NAME).dfs 

$(BUILD_DIR)/spz/:
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) *.z64
	rm -rf filesystem

build_lib:
	rm -rf $(BUILD_DIR) *.z64
	make -C $(T3D_INST)
	make all

test: $(PROJECT_NAME).z64
	ares $<

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
