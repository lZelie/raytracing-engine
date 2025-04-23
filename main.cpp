#include "renderer.h"

int main() {
    gl3::renderer renderer;
    
    // Main render loop
    while (!renderer.should_close()) {
        renderer.render_frame();
    }
    
    return 0;
}