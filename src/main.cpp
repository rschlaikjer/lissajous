#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <sndfile.h>

// Scope colour
static const float colour_r = 0.0;
static const float colour_g = 1.0;
static const float colour_b = 0.2;

float decay_linear(int window_size, int index) {
  // Linear decay map such that
  // index 0 -> alpha 0
  // index window_size-1 -> alpha 1
  return ((float)index) / ((float)window_size);
};

float decay_exp(int window_size, int index) {
  return powf(((float)index) / ((float)window_size), 2);
}

float decay_none(int window_size, int index) {
  // No decay
  return 1.0;
}

void render(float *frames, int window_size, int frame_index, bool invert_lr) {
  // Alright, now it's time to actually do some rendering.
  // How should we decay older points
  auto decay_function = decay_exp;

  // How much should we amplify the signal to fit the screen
  static const float gain = 2.0;

  // For each point in our decay window, calculate the brightness of the point
  // using our decay function and render a nice GL blob at that location
  // Iterate the points in this window
  glLineWidth(4.0);
  glBegin(GL_LINE_STRIP);
  for (int w = frame_index; w < frame_index + window_size; w++) {
    // Update the alpha with the decay for this point
    const float alpha = decay_function(window_size, w - frame_index);
    glColor4f(colour_r, colour_g, colour_b, alpha);
    // Load the left/right channels as x/y
    float x = invert_lr ? frames[w * 2 + 1] : frames[w * 2];
    float y = invert_lr ? frames[w * 2] : frames[w * 2 + 1];
    // Draw the point
    glVertex3f(gain * x, gain * y, 0);
  }
  glEnd();
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "%s [audio file]\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Ensure we can init glfw
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize glfw\n");
    return EXIT_FAILURE;
  }

  // Try and load up the audio infile
  SF_INFO audio_info{};
  SNDFILE *audio = sf_open(argv[1], SFM_READ, &audio_info);
  if (audio == nullptr) {
    fprintf(stderr, "Failed to open file '%s': %s\n", argv[1],
            sf_strerror(nullptr));
    return EXIT_FAILURE;
  }

  // Check that the file looks reasonable
  fprintf(stderr,
          "Loaded input file %s\n"
          "  - Samplerate: %d\n"
          "  - Channels: %d\n",
          argv[1], audio_info.samplerate, audio_info.channels);
  if (audio_info.channels != 2) {
    fprintf(stderr, "Input audio does not have exactly two channels - cannot "
                    "proceed with X/Y Lissajous rendering!\n");
    sf_close(audio);
    return EXIT_FAILURE;
  }

  // Allocate memory for the entire sound file
  // There are two channels per frame, so need frame count * 2 space
  const int buffer_size_bytes = sizeof(float) * 2 * audio_info.frames;
  float *frames = static_cast<float *>(malloc(buffer_size_bytes));
  if (frames == nullptr) {
    fprintf(stderr, "Failed to allocate space for audio data (%d bytes)\n",
            buffer_size_bytes);
    sf_close(audio);
    return EXIT_FAILURE;
  }

  // Parse all that data in
  fprintf(stderr, "Loading audio frame information...\n");
  if (sf_readf_float(audio, frames, audio_info.frames) != audio_info.frames) {
    fprintf(stderr, "Failed to load raw audio frames\n");
    sf_close(audio);
    free(frames);
    return EXIT_FAILURE;
  }
  fprintf(stderr, "%ld frames loaded.\n", audio_info.frames);

  // At this point we can close our sf handle
  sf_close(audio);

  // Time to do some GL setup
  GLFWwindow *window = glfwCreateWindow(1920, 1080, "Lissajous", NULL, NULL);
  if (window == nullptr) {
    fprintf(stderr, "Failed to create glfw window\n");
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Use the sample rate to determine how long each frame should be
  const double frame_time = 1.0 / ((double)audio_info.samplerate);
  fprintf(stderr, "Frame time: %f\n", frame_time);

  double last_frame_time = glfwGetTime();

  // Should we pause frame progression?
  static bool paused = false;
  // Should we swap the L/R X/Y mapping?
  static bool invert_lr = false;
  // Current frame index
  static const int scrub_size_large = audio_info.samplerate;
  static const int scrub_size_small = scrub_size_large / 10;
  static int frame_index = 0;

  // Set a key callback
  glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode,
                                int action, int mods) {
    // Space: pause/play
    if (action == GLFW_PRESS && key == GLFW_KEY_SPACE) {
      paused = !paused;
    }

    // Forward slash: toggle X/Y mapping
    if (action == GLFW_PRESS && key == GLFW_KEY_SLASH) {
      invert_lr = !invert_lr;
    }

    // Left/right arrow keys scrub
    // If shift is held, use the large scrub size.
    bool shift_pressed =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    int scrub_size = shift_pressed ? scrub_size_large : scrub_size_small;
    if (key == GLFW_KEY_LEFT &&
        (action == GLFW_PRESS || action == GLFW_REPEAT)) {
      frame_index -= scrub_size;
      if (frame_index < 0)
        frame_index = 0;
    }

    if (key == GLFW_KEY_RIGHT &&
        (action == GLFW_PRESS || action == GLFW_REPEAT)) {
      frame_index += scrub_size;
      // Don't need to bound check here since main loop will exit
    }
  });

  // Use a 50ms window
  const int window_size = audio_info.samplerate / 20;

  while (!glfwWindowShouldClose(window)) {
    // Init the viewport for the render method
    glClear(GL_COLOR_BUFFER_BIT);
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Check if we have hit the end of the track
    if (frame_index + window_size >= audio_info.frames) {
      // End of track!
      break;
    }

    // Render
    render(frames, window_size, frame_index, invert_lr);

    // Advance the frame index
    double time = glfwGetTime();
    if (!paused) {
      double time_delta = time - last_frame_time;
      int frame_advance_count = time_delta / frame_time;
      frame_index += frame_advance_count;
      if (frame_index > audio_info.frames) {
        break;
      }
    }
    last_frame_time = time;
    fprintf(stderr, "Frame %d/%ld (%0.1f%%)   \r", frame_index,
            audio_info.frames,
            ((float)frame_index) * 100 / ((float)audio_info.frames));

    // Update dispaly & handle glfw events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Clean up glfw
  glfwDestroyWindow(window);
  glfwTerminate();

  free(frames);
  return EXIT_SUCCESS;
}
