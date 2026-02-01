#include <vector>
#include <string>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include "engine_filesystem.h"
#include "engine_t3dm_wrapper.h"

bool T3DMWModel::load(const char* filepath) {
    if(!transform_fp){
        transform_fp = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP) * 6); // allocate one matrix for each framebuffer
    }
    for(int i = 0; i < 6; i++){
        t3d_mat4fp_from_srt_euler(&transform_fp[i],
        {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
        );
    }
    

    model = t3d_model_load(filepath);
    objects.clear();
    if(model){
        T3DModelIter iter;
        iter = t3d_model_iter_create(model, T3D_CHUNK_TYPE_OBJECT);
        while(t3d_model_iter_next(&iter)){
            ModelObject obj;
            obj.object = iter.object;

            if(iter.object->material){
                obj.renderorder = 0;
                obj.hscroll = 0.0f;
                obj.vscroll = 0.0f;
                obj.zread = true;
                obj.zwrite = true;
                obj.fog = 1;

                const char* mat = iter.object->material->name;
                std::string s(mat);
                size_t pos = 0;
                while (pos < s.size()) {
                    // find next token
                    size_t end = s.find_first_of(" ,;", pos);
                    if (end == std::string::npos) end = s.size();
                    std::string token = s.substr(pos, end - pos);
                    pos = (end == s.size()) ? end : end + 1;

                    // split key and value by '=' or ':'
                    size_t sep = token.find_first_of("=:");
                    if (sep == std::string::npos) continue;

                    std::string key = token.substr(0, sep);
                    std::string val = token.substr(sep + 1);

                    // normalize key to lowercase
                    for (char &c : key) c = (char)std::tolower((unsigned char)c);

                auto parse_int = [](const std::string& s, int d)->int{
                    const char* c = s.c_str();
                    char* e = nullptr;
                    long v = std::strtol(c, &e, 0);
                    if (!c[0] || (e && *e)) return d;
                    return (int)v;
                };
                auto parse_float = [](const std::string& s, float d)->float{
                    const char* c = s.c_str();
                    char* e = nullptr;
                    float v = std::strtof(c, &e);
                    if (!c[0] || (e && *e)) return d;
                    return v;
                };
                if (key == "-r" || key == "-renderorder") {
                    obj.renderorder = parse_int(val, obj.renderorder);
                } else if (key == "-hs") {
                    obj.hscroll = parse_float(val, obj.hscroll);
                } else if (key == "-vs") {
                    obj.vscroll = parse_float(val, obj.vscroll);
                } else if (key == "-zr") {
                    obj.zread = parse_int(val, obj.zread) != 0;
                } else if (key == "-zw") {
                    obj.zwrite = parse_int(val, obj.zwrite) != 0;
                } else if (key == "-fog") {
                    obj.fog = parse_int(val, obj.fog);
                }
                }
            }

            objects.push_back(obj);
        }
        std::sort(objects.begin(), objects.end(), [](const ModelObject& a, const ModelObject& b) {
            return a.renderorder < b.renderorder;
        });
    }

    return model != nullptr;
}

void T3DMWModel::draw() {
    if (model) {
        t3d_matrix_push(&transform_fp[FRAME_NUMBER % 6]);
        state = t3d_model_state_create();

        for(const auto& obj : objects) {
            if(!obj.object->userBlock){
                rspq_block_begin();
                t3d_model_draw_material(obj.object->material, &state);  
                rdpq_set_tile_size(TILE0, obj.hscroll, obj.vscroll, 1000, 1000);
                switch(obj.fog){
                    case 0:
                        t3d_fog_set_enabled(false);
                        break;
                    case 1:
                        t3d_fog_set_enabled(true);
                        t3d_state_set_drawflags(static_cast<T3DDrawFlags>(obj.object->material->renderFlags | T3D_FLAG_SHADED));
                        rdpq_mode_blender(RDPQ_BLENDER((IN_RGB, SHADE_ALPHA, FOG_RGB, INV_MUX_ALPHA)));
                        break;
                    case 2:
                        t3d_fog_set_enabled(false);
                        t3d_state_set_drawflags(static_cast<T3DDrawFlags>(obj.object->material->renderFlags | T3D_FLAG_SHADED));
                        rdpq_mode_blender(RDPQ_BLENDER((IN_RGB, SHADE_ALPHA, FOG_RGB, INV_MUX_ALPHA)));
                        break;
                    default:
                        t3d_fog_set_enabled(false);
                        break;  
                }
                rdpq_mode_zbuf(obj.zread, obj.zwrite);
                rdpq_mode_antialias(AA_REDUCED);
                rdpq_mode_zmode(ZMODE_INTERPENETRATING);
                debugf("Drawing object: %s (renderorder=%d, zread=%d, zwrite=%d, hscroll=%.2f, vscroll=%.2f)\n",
                   obj.object->material->name, obj.renderorder, obj.zread, obj.zwrite, obj.hscroll, obj.vscroll);
                t3d_model_draw_object(obj.object, NULL);
                obj.object->userBlock = rspq_block_end();
            } rspq_block_run(obj.object->userBlock);
        }
        t3d_matrix_pop(1);
    }
}

T3DMWModel::T3DMWModel() {
    model = nullptr;
    transform_fp = nullptr;
}

T3DMWModel::~T3DMWModel() {
    free();
}

void T3DMWModel::free() {
    if (model) {
        t3d_model_free(model);
        model = nullptr;
        objects.clear(); // Clear objects vector to prevent stale data
        debugf("T3DMWModel freed\n");
    }
    if (transform_fp) {
        free_uncached(transform_fp);
        transform_fp = nullptr;
    }
}
