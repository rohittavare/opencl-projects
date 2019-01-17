#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <CL/cl.h>

int arr_size = 10;

int main(int argc, char ** argv) {
	struct option * opt = (struct option *) malloc(2 * sizeof(struct option));
	opt[0].name = "length";
	opt[0].val = 'l';
	opt[0].has_arg = required_argument;
	opt[0].flag = 0;
	opt[1].name = NULL;
	opt[1].val = 0;
	opt[1].has_arg = 0;
	opt[1].flag = NULL;

	//read in our command line arguments
	char ch;
	int ind;
	while((ch = getopt_long(argc, argv, "l:", opt, &ind))) {
		if(ch == -1) break;
		switch(ch) {
			case 'l':
				arr_size = strtoimax(optarg, NULL, 10);
				if(arr_size == 0 || errno == ERANGE) { fprintf(stderr, "error: invalid array length\n"); exit(1); }
				break;
			case '?':
				exit(1);
		}
	}
	free(opt);

	// we got to read in the source code of the kernel
	int fd = open("kernel.cl", O_RDONLY);
	char * source_code = (char *) malloc(300 * sizeof(char));
	size_t source_len = read(fd, source_code, 300);
	
	// create our data
	int * a = (int *) malloc(arr_size * sizeof(int));
	int * b = (int *) malloc(arr_size * sizeof(int));
	int * c = (int *) malloc(arr_size * sizeof(int));
	int j;
	for(j = 0; j < arr_size; j++) {
		a[j] = j;
		b[j] = arr_size - j;
	}

	// query our devices and create a context
	cl_platform_id * platform = NULL;
	cl_uint num_platforms;
	cl_device_id * device = NULL;
	cl_uint num_devices;
	cl_int err;
	
	err = clGetPlatformIDs(0, NULL, &num_platforms);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not query platform\n"); exit(1); }
	platform = malloc(num_platforms * sizeof(cl_platform_id));
	err = clGetPlatformIDs(num_platforms, platform, NULL);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not query platform\n"); exit(1); }
	
	err = clGetDeviceIDs(*platform, CL_DEVICE_TYPE_DEFAULT, 0, NULL, &num_devices);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not query device\n"); exit(1); }
	device = malloc(num_devices * sizeof(cl_device_id));
	err = clGetDeviceIDs(*platform, CL_DEVICE_TYPE_DEFAULT, num_devices, device, NULL);// get the device id's for the first platform
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not query device\n"); exit(1); }

	cl_context context = clCreateContext(NULL, 1, device, NULL, NULL, &err);// create the context for one device
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create context"); exit(1); }
	
	// create our queue
	cl_command_queue queue = clCreateCommandQueueWithProperties( context, *device, NULL, &err);// create a command queue for one device
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create queue\n"); exit(1); }

	// create and compile our programs
	cl_program program = clCreateProgramWithSource(context, 1, (const char **) &source_code, &source_len, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create program from source code\n"); exit(1); }
	err = clBuildProgram(program, 1, device, NULL, NULL, NULL);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not build program\n"); exit(1); }

	// create our buffers and copy into them
	cl_mem A = clCreateBuffer(context, CL_MEM_READ_ONLY, arr_size * sizeof(int), NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create buffer A\n"); exit(1); }
	cl_mem B = clCreateBuffer(context, CL_MEM_READ_ONLY, arr_size * sizeof(int), NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create buffer B\n"); exit(1); }
	cl_mem C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, arr_size * sizeof(int), NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create buffer C\n"); exit(1); }

	err = clEnqueueWriteBuffer(queue, A, CL_TRUE, 0, arr_size * sizeof(int), a, 0, NULL, NULL);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not copy buffer a to A\n"); exit(1); }
	err = clEnqueueWriteBuffer(queue, B, CL_TRUE, 0, arr_size * sizeof(int), b, 0, NULL, NULL);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not copy buffer b to B\n"); exit(1); }

	// create our kernel
	cl_kernel kernel = clCreateKernel(program, "vector_addition", &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create kernel\n"); exit(1); }
	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &A);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not set first argument to kernel\n"); exit(1); }
	err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &B);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not set second argument to kernel\n"); exit(1); }
	err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &C);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not set third argument to kernel\n"); exit(1); }

	// run the kernel
	size_t num_work_groups = arr_size;
	size_t num_work_items_per_group = 32;
	err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &num_work_groups, &num_work_items_per_group, 0, NULL, NULL);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not enqueue kernel\n"); exit(1); }

	// read in the result
	err = clEnqueueReadBuffer(queue, C, CL_TRUE, 0, arr_size * sizeof(int), c, 0, NULL, NULL);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not read from buffer C to c\n"); exit(1); }
	
	// output
	printf("vector length: %i (default 10)\n", arr_size);
	printf("vector A: ");
	int i;
	for(i = 0; i < arr_size; i++) {
		printf("%i ", a[i]);
	}
	printf("\n");
	printf("vector B: ");
	for(i = 0; i < arr_size; i++) {
		printf("%i ", b[i]);
	}
	printf("\n");
	printf("vector C: ");
	for(i = 0; i < arr_size; i++) {
		printf("%i ", c[i]);
	}
	printf("\n");
	
	// clear opencl resources
	err = clFlush(queue);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not flush the queue\n"); exit(1); }
	err = clFinish(queue);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not finish the queue\n"); exit(1); }
	err = clReleaseKernel(kernel);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release the kernel\n"); exit(1); }
	err = clReleaseMemObject(A);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release memory for buffer A\n"); exit(1); }
	err = clReleaseMemObject(B);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release memory for buffer B\n"); exit(1); }
	err = clReleaseMemObject(C);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release memory for buffer C\n"); exit(1); }
	err = clReleaseProgram(program);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release program\n"); exit(1); }
	err = clReleaseCommandQueue(queue);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release queue\n"); exit(1); }
	err = clReleaseContext(context);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release context\n"); exit(1); }
	
	// clear our resources
	free(device);
	free(platform);
	free(source_code);
	free(a);
	free(b);
	free(c);
}
