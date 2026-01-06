#ifndef ENGINE_T3DM_WRAPPER
#define ENGINE_T3DM_WRAPPER
/// T3DM wrapper functions

#include <vector>
#include <string>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include "engine_filesystem.h"

class T3DMWModel {
    public:
        T3DMat4FP* transform_fp;
        T3DModel* model;
        T3DModelState state;

        struct ModelObject{
            int renderorder;
            bool zread, zwrite;
            float hscroll,vscroll;
            int fog;
            T3DObject* object;
        };
        std::vector<ModelObject> objects;

        T3DMWModel();
        ~T3DMWModel();

        T3DMat4FP* get_transform_fp(){
            return &transform_fp[FRAME_NUMBER % 3];
        };

        void set_transform_fp(T3DMat4FP &mat){
            *get_transform_fp() = mat;
        }

        bool load(const char* filepath);
        void free();
    
        void draw();

};

#endif