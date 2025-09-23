#include <iostream>
#include <vector>
#include <boost/compute.hpp>
//#include "~/misc/boost.compute/include/boost/conpute.hpp"
#include <cmath>

namespace compute = boost::compute;

// Struct to represent a 2D Gaussian, with padding to ensure alignment for OpenCL
struct Gaussian {
    compute::float_ mean[2];
    compute::float_ cov[4];
    compute::float_ color[3];
    compute::float_ alpha;
    compute::float_ padding[3]; // Pad to 32 bytes to avoid alignment issues
};

// OpenCL kernel source code (can also be loaded from a file)
// NOTE: OpenCL C99 does not support structs with nested arrays and matrices directly.
// We must declare the Gaussian struct manually in the kernel.
const char *gaussian_splat_kernel_source = BOOST_COMPUTE_STRINGIZE_SOURCE(
    typedef struct {
        float mean[2];
        float cov[4];
        float color[3];
        float alpha;
        float padding[3];
    } Gaussian;

    float gaussian_value(float x, float y, __global const Gaussian *g) {
        float det_inv = 1.0f / (g->cov[0] * g->cov[3] - g->cov[1] * g->cov[2]);
        float inv_cov[4] = {
            g->cov[3] * det_inv,
            -g->cov[1] * det_inv,
            -g->cov[2] * det_inv,
            g->cov[0] * det_inv
        };

        float dx = x - g->mean[0];
        float dy = y - g->mean[1];
        
        float val = (dx * inv_cov[0] + dy * inv_cov[2]) * dx +
                    (dx * inv_cov[1] + dy * inv_cov[3]) * dy;
        return exp(-0.5f * val);
    }

    __kernel void gaussian_splat(
        __global const Gaussian *gaussians,
        __global unsigned char *output_image,
        const int num_gaussians,
        const int width,
        const int height) 
    {
        // Global ID of the current thread (pixel index)
        int i = get_global_id(0);
        int j = get_global_id(1);

        if (i >= width || j >= height) {
            return;
        }

        float final_r = 0.0f;
        float final_g = 0.0f;
        float final_b = 0.0f;
        float final_a = 0.0f;

        // Iterate through all Gaussians
        for (int k = 0; k < num_gaussians; ++k) {
            __global const Gaussian *g = &gaussians[k];
            
            // Simplified bounding box check for optimization
            // Can be expanded to be more accurate
            float distance_sq = pow(i - g->mean[0], 2) + pow(j - g->mean[1], 2);
            if (distance_sq > 2500) { // arbitrary threshold
                continue;
            }

            float gauss_val = gaussian_value(i, j, g);
            float alpha = g->alpha * gauss_val;

            // Alpha blending (back-to-front)
            final_r = final_r * (1.0f - alpha) + g->color[0] * alpha;
            final_g = final_g * (1.0f - alpha) + g->color[1] * alpha;
            final_b = final_b * (1.0f - alpha) + g->color[2] * alpha;
            final_a = final_a + alpha * (1.0f - final_a);
        }

        // Convert float color to byte values
        int idx = (j * width + i) * 4;
        output_image[idx]     = (unsigned char)(final_r * 255.0f);
        output_image[idx + 1] = (unsigned char)(final_g * 255.0f);
        output_image[idx + 2] = (unsigned char)(final_b * 255.0f);
        output_image[idx + 3] = (unsigned char)(final_a * 255.0f);
    }
);

int main() {
    // 1. Setup Boost.Compute and OpenCL
    try {
        compute::device device = compute::system::default_device();
        compute::context context(device);
        compute::command_queue queue(context, device);

        // 2. Define image dimensions and Gaussians
        const int width = 1024;
        const int height = 768;
        const int num_gaussians = 5000;

        // Create a vector to hold your Gaussians on the host (CPU)
        std::vector<Gaussian> host_gaussians(num_gaussians);
        
        // TODO: Populate your `host_gaussians` array here with realistic data.
        // For demonstration, we'll generate some random Gaussians.
        for (int i = 0; i < num_gaussians; ++i) {
            host_gaussians[i].mean[0] = static_cast<float>(rand() % width);
            host_gaussians[i].mean[1] = static_cast<float>(rand() % height);
            
            host_gaussians[i].cov[0] = 50.0f; // Covariance xx
            host_gaussians[i].cov[1] = 0.0f;  // Covariance xy
            host_gaussians[i].cov[2] = 0.0f;  // Covariance yx
            host_gaussians[i].cov[3] = 50.0f; // Covariance yy
            
            host_gaussians[i].color[0] = static_cast<float>(rand() % 256) / 255.0f;
            host_gaussians[i].color[1] = static_cast<float>(rand() % 256) / 255.0f;
            host_gaussians[i].color[2] = static_cast<float>(rand() % 256) / 255.0f;
            host_gaussians[i].alpha = 0.5f;
        }

        // 3. Create and transfer data to device (GPU) memory
        compute::vector<Gaussian> device_gaussians(num_gaussians, context);
        compute::copy(
            host_gaussians.begin(),
            host_gaussians.end(),
            device_gaussians.begin(),
            queue
        );

        // Output image buffer on the device
        compute::vector<unsigned char> device_image(width * height * 4, context);
        
        // 4. Compile and build the OpenCL kernel
        compute::program program = compute::program::create_with_source(
            gaussian_splat_kernel_source,
            context
        );
        program.build();
        
        compute::kernel gaussian_splat_kernel(
            program.create_kernel("gaussian_splat")
        );
        
        // 5. Set kernel arguments
        gaussian_splat_kernel.set_arg(0, device_gaussians);
        gaussian_splat_kernel.set_arg(1, device_image);
        gaussian_splat_kernel.set_arg(2, num_gaussians);
        gaussian_splat_kernel.set_arg(3, width);
        gaussian_splat_kernel.set_arg(4, height);

        // 6. Run the kernel
        // Define the global and local work sizes
        size_t global_work_size[] = {
            static_cast<size_t>(width),
            static_cast<size_t>(height)
        };
        size_t local_work_size[] = {
            16,
            16
        };

        std::cout << "Starting GPU computation..." << std::endl;
        queue.enqueue_nd_range_kernel(
            gaussian_splat_kernel,
            2,
            0,
            global_work_size,
            local_work_size
        );
        
        // Wait for the kernel to finish
        queue.finish();
        std::cout << "GPU computation finished." << std::endl;

        // 7. Read the result back to the host (CPU)
        std::vector<unsigned char> host_image(width * height * 4);
        compute::copy(
            device_image.begin(),
            device_image.end(),
            host_image.begin(),
            queue
        );
        
        // Now `host_image` contains the final rendered image data in RGBA format
        // You can save it to a file (e.g., using a library like stb_image_write)
        // or display it using your existing rendering framework.
        
        std::cout << "Image data copied to host." << std::endl;

    } catch (const compute::opencl_error& e) {
        std::cerr << "OpenCL Error: " << e.what() << " (" << e.error_string() << ")" << std::endl;
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Standard Exception: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
