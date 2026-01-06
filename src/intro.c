#include <libdragon.h>
#include "utils.h"
#include "intro.h"


void libdragon_logo()
{
    const color_t RED = RGBA32(221, 46, 26, 255);
    const color_t WHITE = RGBA32(255, 255, 255, 255);

    sprite_t *d1 = sprite_load("rom:/textures/intro/dragon1.i8.sprite");
    sprite_t *d2 = sprite_load("rom:/textures/intro/dragon2.i8.sprite");
    sprite_t *d3 = sprite_load("rom:/textures/intro/dragon3.i8.sprite");
    sprite_t *d4 = sprite_load("rom:/textures/intro/dragon4.i8.sprite");
    wav64_t music;
    wav64_open(&music, "rom:/sfx/intro/dragon.wav64");
    mixer_ch_set_limits(0, 0, 48000, 0);

    display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);

    float angle1 = 0, angle2 = 0, angle3 = 0;
    float scale1 = 0, scale2 = 0, scale3 = 0, scroll4 = 0;
    uint32_t ms0 = 0;
    int anim_part = 0;
    const int X0 = 10, Y0 = 30; // translation offset of the animation (simplify centering)

    void reset() {
        ms0 = get_ticks_ms();
        anim_part = 0;

        angle1 = 3.2f;
        angle2 = 1.9f;
        angle3 = 0.9f;
        scale1 = 0.0f;
        scale2 = 0.4f;
        scale3 = 0.8f;
        scroll4 = 400;
        wav64_play(&music, 0);
    }

    reset();
    while (1) {
        mixer_try_play();
        
        // Calculate animation part:
        // 0: rotate dragon head
        // 1: rotate dragon body and tail, scale up
        // 2: scroll dragon logo
        // 3: fade out 
        uint32_t tt = get_ticks_ms() - ms0;
        if (tt < 1000) anim_part = 0;
        else if (tt < 1500) anim_part = 1;
        else if (tt < 4000) anim_part = 2;
        else if (tt < 5000) anim_part = 3;
        else break;

        // Update animation parameters using quadratic ease-out
        angle1 -= angle1 * 0.04f; if (angle1 < 0.010f) angle1 = 0;
        if (anim_part >= 1) {
            angle2 -= angle2 * 0.06f; if (angle2 < 0.01f) angle2 = 0;
            angle3 -= angle3 * 0.06f; if (angle3 < 0.01f) angle3 = 0;
            scale2 -= scale2 * 0.06f; if (scale2 < 0.01f) scale2 = 0;
            scale3 -= scale3 * 0.06f; if (scale3 < 0.01f) scale3 = 0;
        }
        if (anim_part >= 2) {
            scroll4 -= scroll4 * 0.08f;
        }

        // Update colors for fade out effect
        color_t red = RED;
        color_t white = WHITE;
        if (anim_part >= 3) {
            red.a = 255 - (tt-4000) * 255 / 1000;
            white.a = 255 - (tt-4000) * 255 / 1000;
        }

        #if 0
        // Debug: re-run logo animation on button press
        joypad_poll();
        joypad_buttons_t btn = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if (btn.a) reset();
        #endif

        surface_t *fb = display_get();
        rdpq_attach_clear(fb, NULL);

        // To simulate the dragon jumping out, we scissor the head so that
        // it appears as it moves.
        if (angle1 > 1.0f) {
            // Initially, also scissor horizontally, 
            // so that the head tail is not visible on the right.
            rdpq_set_scissor(0, 0, X0+300, Y0+240);    
        } else {
            rdpq_set_scissor(0, 0, 640, Y0+240);
        }

        // Draw dragon head
        rdpq_set_mode_standard();
        rdpq_mode_alphacompare(1);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(TEX0,0,PRIM,0)));
        rdpq_set_prim_color(red);
        rdpq_sprite_blit(d1, X0+216, Y0+205, &(rdpq_blitparms_t){ 
            .theta = angle1, .scale_x = scale1+1, .scale_y = scale1+1,
            .cx = 176, .cy = 171,
        });

        // Restore scissor to standard
        rdpq_set_scissor(0, 0, 640, 480);

        // Draw a black rectangle with alpha gradient, to cover the head tail
        rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
        rdpq_mode_dithering(DITHER_NOISE_NOISE);
        float vtx[4][6] = {
            //  x,      y,    r,g,b,a
            { X0+0,   Y0+180, 0,0,0,0 },
            { X0+200, Y0+180, 0,0,0,0 },
            { X0+200, Y0+240, 0,0,0,1 },
            { X0+0,   Y0+240, 0,0,0,1 },
        };
        rdpq_triangle(&TRIFMT_SHADE, vtx[0], vtx[1], vtx[2]);
        rdpq_triangle(&TRIFMT_SHADE, vtx[0], vtx[2], vtx[3]);

        if (anim_part >= 1) {
            // Draw dragon body and tail
            rdpq_set_mode_standard();
            rdpq_mode_alphacompare(1);
            rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(TEX0,0,PRIM,0)));

            // Fade them in
            color_t color = red;
            color.r *= 1-scale3; color.g *= 1-scale3; color.b *= 1-scale3;
            rdpq_set_prim_color(color);

            rdpq_sprite_blit(d2, X0+246, Y0+230, &(rdpq_blitparms_t){ 
                .theta = angle2, .scale_x = 1-scale2, .scale_y = 1-scale2,
                .cx = 145, .cy = 113,
            });

            rdpq_sprite_blit(d3, X0+266, Y0+256, &(rdpq_blitparms_t){ 
                .theta = -angle3, .scale_x = 1-scale3, .scale_y = 1-scale3,
                .cx = 91, .cy = 24,
            });
        }

        // Draw scrolling logo
        if (anim_part >= 2) {
            rdpq_set_prim_color(white);
            rdpq_sprite_blit(d4, X0 + 161 + (int)scroll4, Y0 + 182, NULL);
        }

        rdpq_detach_show();
    }

    wait_ms(500); // avoid immediate switch to next screen
    rspq_wait();
    sprite_free(d1);
    sprite_free(d2);
    sprite_free(d3);
    sprite_free(d4);
    wav64_close(&music);
    display_close();
}


// Number of frame back buffers we reserve.
// These buffers are used to render the video ahead of time.
// More buffers help ensure smooth video playback at the cost of more memory.
#define NUM_DISPLAY 3

// Maximum target audio frequency.
//
// Needs to be 48 kHz if Opus audio compression is used.
// In this example, we are using VADPCM audio compression
// which means we can use the real frequency of the audio track.
#define AUDIO_HZ 32000.0f

void movie_play(char* moviefilename, char* audiofilename, float movie_fps)
{
	joypad_init();

	yuv_init();

	// Check if the movie is present in the filesystem, so that we can provide
	// a specific error message.
	FILE *f = fopen(moviefilename, "rb");
	assertf(f, "Movie not found %s!", moviefilename);
	fclose(f);

	// Open the movie using the mpeg2 module and create a YUV blitter to draw it.
	mpeg2_t* video_track = mpeg2_open(moviefilename);

	int video_width = mpeg2_get_width(video_track);
	int video_height = mpeg2_get_height(video_track);

	// When playing back a video, there are essentially two options:
	// 1) Configure a fixed resolution (eg: 320x240), and then make
	//    the video fit it, with letterboxing if necessary. This is requires
	//    actually drawing / rescaling the video with RDP and filling the
	//    rest of the framebuffers with black.
	// 2) Configure a resolution which exactly matches the video resolution,
	//     and let VI perform the necessary centering / letterboxing.
	//
	// 2 is more efficient for full motion videos because no additional memory
	// is wasted in framebuffers to hold black pixels, so we go with it.

	display_init((resolution_t){
			// Initialize a framebuffer resolution which precisely matches the video
			.width = video_width, .height = video_height,
			.interlaced = INTERLACE_HALF,
			// Set the desired aspect ratio to that of the video. By default,
			// display_init would force 4:3 instead, which would be wrong here.
			// eg: if a video is 320x176, we want to display it as 16:9-ish.
			.aspect_ratio = (float)video_width / video_height,
			// Uncomment this line if you want to have some additional black
			// borders to fully display the video on real CRTs.
			// .overscan_margin = VI_CRT_MARGIN,
		},
		// 32-bit display mode is mandatory for video playback.
		DEPTH_16_BPP,
		NUM_DISPLAY, GAMMA_NONE,
		// Activate bilinear filtering while rescaling the video
		FILTERS_DEDITHER
	);

	yuv_blitter_t yuv = yuv_blitter_new_fmv(
		// Resolution of the video we expect to play.
		// Video needs to have a width divisible by 32 and a height divisible by 16.
		video_width, video_height,
		// Set blitter's output area to our entire display. Given the above
		// initialization, this will actually match the video width/height, but
		// if we instead opted for a fixed resolution (eg: 320x240), it would be
		// the YUV blitter that would letterbox the video by adding black borders
		// where necessary.
		display_get_width(), display_get_height(),
		// You can further customize YUV options through this parameter structure
		// if necessary.
		&(yuv_fmv_parms_t) {}
	);

	// Engage the fps limiter to ensure proper video pacing.
	float fps = mpeg2_get_framerate(video_track);
    
    if(movie_fps > 0){
        fps = movie_fps;
    }

	display_set_fps_limit(fps);


	// Open the audio track and start playing it in channel 0.
	wav64_t audio_track;
    if(audiofilename){
        FILE *f = fopen(audiofilename, "rb");
        assertf(f, "Audio not found, but requested %s!", audiofilename);
        fclose(f);

        wav64_open(&audio_track, audiofilename);
        mixer_ch_play(0, &audio_track.wave);
    }


	int nframes = 0;

	while (1)
	{
		mixer_throttle(AUDIO_HZ / fps);

		if (!mpeg2_next_frame(video_track))
		{
			break;
		}

		// This polls the mixer to try and play the next chunk of audio, if available.
		// We call this function twice during the frame to make sure the audio never stalls.
		mixer_try_play();

		rdpq_attach(display_get(), NULL);

		// Get the next video frame and feed it into our previously set up blitter.
		yuv_frame_t frame = mpeg2_get_frame(video_track);
        rdpq_mode_dithering(DITHER_SQUARE_INVSQUARE);
		yuv_blitter_run(&yuv, &frame);

		rdpq_detach_show();

		nframes++;

		mixer_try_play();
	}

    rspq_wait();

    mpeg2_close(video_track);
    yuv_blitter_free(&yuv);
    if(audiofilename){
        wav64_close(&audio_track);
    }
    display_close();
}

void game_logo(){
    sprite_t *logo = sprite_load("rom:/textures/intro/gamelogo.i8.sprite");
    surface_t logosurf = sprite_get_pixels(logo);

	display_init((resolution_t){
			.width = 640, .height = 240,
			.interlaced = INTERLACE_HALF,
			.aspect_ratio = (float)640 / 240,
		},
		DEPTH_16_BPP,
		3, GAMMA_NONE,
		FILTERS_RESAMPLE
	);

    float time = 6.0f; // seconds to display logo
    int xstart, ystart;
    xstart = 320 - logo->width / 2;
    ystart = 120 - logo->height / 2;
    while(time > 0.0f){

        float alpha = 1;
        if(time >= 4) alpha = ((6.0 - time) / 2.0f);
        if(time <= 2) alpha = (time / 2.0f);
        uint8_t col = (alpha) * 255;
        alpha = 1 - alpha;

        surface_t *fb = display_get();
        rdpq_attach_clear(fb, NULL);

        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER1((TEX0,0,PRIM,0),(0,0,0,1)));
        rdpq_set_prim_color(RGBA32(col,col,col,255));

        for(int i = 0; i < logo->height; i++){
            surface_t surf = surface_make_sub(&logosurf, 0, i, logosurf.width, 1);
            int random = frandr(-alpha*40, alpha*40);
            rdpq_tex_blit(&surf, xstart + random, ystart + i, NULL);
        }

        rdpq_detach_show();

        time -= display_get_delta_time();
    }

    rspq_wait();

    sprite_free(logo);
    display_close();
}
