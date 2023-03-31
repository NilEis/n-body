#include "opencl_tick.h"
#include <CL/opencl.h>
#include "opencl_kernel.h"
#include <stdlib.h>
#include "defines.h"

static cl_platform_id platform_id = NULL;
static cl_device_id device_id = NULL;
static cl_uint ret_num_devices = 0;
static cl_uint ret_num_platforms = 0;
static cl_context context = NULL;
static cl_command_queue command_queue = NULL;
static cl_program program = NULL;
static cl_kernel kernel_solve = NULL;
static cl_kernel kernel_update = NULL;

cl_mem cl_particles = NULL;

static size_t work_dim = 128;

static void kernel_error(cl_int ret);
static void arg_error(cl_int ret);
static void enq_error(cl_int ret);

void init_opencl_tick(Particle *p, int n)
{
    cl_int ret = 0;
    LOG(LOG_INFO, "Initialising OPENCL\n");
    ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    if (ret == CL_SUCCESS)
    {
        LOG(LOG_INFO, "clGetPlatformIDs success\n");
    }
    else if (ret != CL_SUCCESS)
    {
        switch (ret)
        {
        case CL_INVALID_VALUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_VALUE: num_entries is equal to zero and platforms is not NULL or both num_platforms and platforms are NULL.\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        case CL_PLATFORM_NOT_FOUND_KHR:
            LOG(LOG_TIME | LOG_FUNC, "CL_PLATFORM_NOT_FOUND_KHR: the cl_khr_icd extension is enabled and no platforms are found.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
            break;
        }
    }
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
    if (ret == CL_SUCCESS)
    {
        LOG(LOG_INFO, "clGetDeviceIDs success\n");
    }
    else if (ret != CL_SUCCESS)
    {
        switch (ret)
        {
        case CL_INVALID_PLATFORM:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_PLATFORM: platform is not a valid platform.\n");
            break;

        case CL_INVALID_DEVICE_TYPE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_DEVICE_TYPE: device_type is not a valid value.\n");
            break;

        case CL_INVALID_VALUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_VALUE: num_entries is equal to zero and devices is not NULL or if both num_devices and devices are NULL.\n");
            break;

        case CL_DEVICE_NOT_FOUND:
            LOG(LOG_TIME | LOG_FUNC, "CL_DEVICE_NOT_FOUND: no OpenCL devices that matched device_type were found.\n");
            break;

        case CL_OUT_OF_RESOURCES:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_RESOURCES: there is a failure to allocate resources required by the OpenCL implementation on the device.\n");
            break;

        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
            break;
        }
    }
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    if (ret == CL_SUCCESS)
    {
        LOG(LOG_INFO, "clCreateContext success\n");
    }
    else if (ret != CL_SUCCESS)
    {
        switch (ret)
        {
        case CL_INVALID_PLATFORM:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_PLATFORM: properties is NULL and no platform could be selected or if platform value specified in properties is not a valid platform.\n");
            break;
        case CL_INVALID_PROPERTY:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_PROPERTY: context property name in properties is not a supported property name, the value specified for a supported property name is not valid, or the same property name is specified more than once.\n");
            break;
        case CL_INVALID_VALUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_VALUE: devices is NULL; num_devices is equal to zero; or pfn_notify is NULL but user_data is not NULL.\n");
            break;
        case CL_INVALID_DEVICE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_DEVICE: devices contains an invalid device.\n");
            break;
        case CL_DEVICE_NOT_AVAILABLE:
            LOG(LOG_TIME | LOG_FUNC, "CL_DEVICE_NOT_AVAILABLE: a device in devices is currently not available even though the device was returned by clGetDeviceIDs.\n");
            break;
        case CL_OUT_OF_RESOURCES:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_RESOURCES: there is a failure to allocate resources required by the OpenCL implementation on the device.\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
            break;
        }
    }
    command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);
    if (ret == CL_SUCCESS)
    {
        LOG(LOG_INFO, "clCreateCommandQueueWithProperties success\n");
    }
    else if (ret != CL_SUCCESS)
    {
        switch (ret)
        {
        case CL_INVALID_CONTEXT:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_CONTEXT: context is not a valid context.\n");
            break;
        case CL_INVALID_DEVICE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_DEVICE: device is not a valid device or is not associated with context.\n");
            break;
        case CL_INVALID_VALUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_VALUE: values specified in properties are not valid.\n");
            break;
        case CL_INVALID_QUEUE_PROPERTIES:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_QUEUE_PROPERTIES: values specified in properties are valid but are not supported by the device.\n");
            break;
        case CL_OUT_OF_RESOURCES:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_RESOURCES: there is a failure to allocate resources required by the OpenCL implementation on the device.\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
            break;
        }
    }
    //cl_particles = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR, n * sizeof(Particle), (void *)p, &ret);
    cl_particles = clCreateBuffer(context, CL_MEM_READ_WRITE, n * sizeof(Particle), NULL, &ret);
    if (ret == CL_SUCCESS)
    {
        LOG(LOG_INFO, "clCreateBuffer success - %d bytes\n", n * sizeof(Particle));
    }
    else if (ret != CL_SUCCESS)
    {
        switch (ret)
        {
        case CL_INVALID_CONTEXT:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_CONTEXT: context is not a valid context.\n");
            break;
        case CL_INVALID_VALUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_VALUE: values specified in flags are not valid as defined in the table above.\n");
            break;
        case CL_INVALID_BUFFER_SIZE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_BUFFER_SIZE: size is 0.\n");
            break;
        case CL_INVALID_HOST_PTR:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_HOST_PTR: host_ptr is NULL and CL_MEM_USE_HOST_PTR or CL_MEM_COPY_HOST_PTR are set in flags or host_ptr is not NULL but CL_MEM_COPY_HOST_PTR or CL_MEM_USE_HOST_PTR are not set in flags.\n");
            break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            LOG(LOG_TIME | LOG_FUNC, "CL_MEM_OBJECT_ALLOCATION_FAILURE: there is a failure to allocate memory for buffer object.\n");
            break;
        case CL_OUT_OF_RESOURCES:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_RESOURCES: there is a failure to allocate resources required by the OpenCL implementation on the device.\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
            break;
        }
    }
    ret = clEnqueueWriteBuffer(command_queue, cl_particles, CL_TRUE, 0, n * sizeof(Particle), p, 0, NULL, NULL);
    if (ret == CL_SUCCESS)
    {
        LOG(LOG_INFO, "clEnqueueWriteBuffer success\n");
    }
    else if (ret != CL_SUCCESS)
    {
        switch (ret)
        {

        case CL_INVALID_COMMAND_QUEUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_COMMAND_QUEUE: command_queue is not a valid command-queue.\n");
            break;
        case CL_INVALID_CONTEXT:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_CONTEXT: the context associated with command_queue and buffer are not the same or if the context associated with command_queue and events in event_wait_list are not the same.\n");
            break;
        case CL_INVALID_MEM_OBJECT:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_MEM_OBJECT: buffer is not a valid buffer object.\n");
            break;
        case CL_INVALID_VALUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_VALUE: the region being written specified by (offset, size) is out of bounds or if ptr is a NULL value.\n");
            break;
        case CL_INVALID_EVENT_WAIT_LIST:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_EVENT_WAIT_LIST: event_wait_list is NULL and num_events_in_wait_list > 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or event objects in event_wait_list are not valid events.\n");
            break;
        case CL_MISALIGNED_SUB_BUFFER_OFFSET:
            LOG(LOG_TIME | LOG_FUNC, "CL_MISALIGNED_SUB_BUFFER_OFFSET: buffer is a sub-buffer object and offset specified when the sub-buffer object is created is not aligned to CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with queue.\n");
            break;
        case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
            LOG(LOG_TIME | LOG_FUNC, "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: the read and write operations are blocking and the execution status of any of the events in event_wait_list is a negative integer value.\n");
            break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            LOG(LOG_TIME | LOG_FUNC, "CL_MEM_OBJECT_ALLOCATION_FAILURE: there is a failure to allocate memory for data store associated with buffer.\n");
            break;
        case CL_INVALID_OPERATION:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_OPERATION: clEnqueueWriteBuffer is called on buffer which has been created with CL_MEM_HOST_READ_ONLY or CL_MEM_HOST_NO_ACCESS.\n");
            break;
        case CL_OUT_OF_RESOURCES:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_RESOURCES: there is a failure to allocate resources required by the OpenCL implementation on the device.\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
            break;
        }
    }
    program = clCreateProgramWithSource(context, 1, (const char **)&opencl_kernel_source, (const size_t *)&opencl_kernel_source_size, &ret);
    if (ret == CL_SUCCESS)
    {
        LOG(LOG_INFO, "clCreateProgramWithSource success\n");
    }
    else if (ret != CL_SUCCESS)
    {
        switch (ret)
        {
        case CL_INVALID_CONTEXT:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_CONTEXT: context is not a valid context.\n");
            break;
        case CL_INVALID_VALUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_VALUE: count is zero or strings or any entry in strings is NULL.\n");
            break;
        case CL_OUT_OF_RESOURCES:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_RESOURCES: there is a failure to allocate resources required by the OpenCL implementation on the device.\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
            break;
        }
    }
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (ret == CL_SUCCESS)
    {
        LOG(LOG_INFO, "clBuildProgram success\n");
    }
    else if (ret != CL_SUCCESS)
    {
        switch (ret)
        {
        case CL_INVALID_PROGRAM:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_PROGRAM: program is not a valid program object.\n");
            break;
        case CL_INVALID_VALUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_VALUE: device_list is NULL and num_devices is greater than zero, or device_list is not NULL and num_devices is zero or pfn_notify is NULL but user_data is not NULL.\n");
            break;
        case CL_INVALID_DEVICE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_DEVICE: OpenCL devices listed in device_list are not in the list of devices associated with program.\n");
            break;
        case CL_INVALID_BINARY:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_BINARY: program is created with clCreateProgramWithBinary and devices listed in device_list do not have a valid program binary loaded.\n");
            break;
        case CL_INVALID_BUILD_OPTIONS:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_BUILD_OPTIONS: the build options specified by options are invalid.\n");
            break;
        case CL_INVALID_OPERATION:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_OPERATION: the build of a program executable for any of the devices listed in device_list by a previous call to clBuildProgram for program has not completed or there are kernel objects attached to program or program was not created with clCreateProgramWithSource, clCreateProgramWithIL, or clCreateProgramWithBinary or the program requires independent forward progress of sub-groups but one or more of the devices listed in device_list does not return CL_TRUE for the CL_DEVICE_SUBGROUP_INDEPENDENT_FORWARD_PROGRESS query.\n");
            break;
        case CL_COMPILER_NOT_AVAILABLE:
            LOG(LOG_TIME | LOG_FUNC, "CL_COMPILER_NOT_AVAILABLE: program is created with clCreateProgramWithSource and a compiler is not available i.e. CL_DEVICE_COMPILER_AVAILABLE specified in the table of OpenCL Device Queries for clGetDeviceInfo is set to CL_FALSE.\n");
            break;
        case CL_BUILD_PROGRAM_FAILURE:
            LOG(LOG_TIME | LOG_FUNC, "CL_BUILD_PROGRAM_FAILURE: there is a failure to build the program executable. This error will be returned if clBuildProgram does not return until the build has completed.\n");
            break;
        case CL_OUT_OF_RESOURCES:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_RESOURCES: there is a failure to allocate resources required by the OpenCL implementation on the device.\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
            break;
        }
    }
    LOG(LOG_INFO, "Creating Kernel\n");
    kernel_solve = clCreateKernel(program, "newtonianSolver", &ret);
    if (ret == CL_SUCCESS)
    {
        LOG(LOG_INFO, "clCreateKernel success\n");
    }
    kernel_error(ret);
    kernel_update = clCreateKernel(program, "update_particle", &ret);
    if (ret == CL_SUCCESS)
    {
        LOG(LOG_INFO, "clCreateKernel success\n");
    }
    kernel_error(ret);
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &work_dim, &ret);
}

static void arg_error(cl_int ret)
{
    if (ret != CL_SUCCESS)
    {
        switch (ret)
        {
        case CL_INVALID_KERNEL:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_KERNEL: kernel is not a valid kernel object.\n");
            break;
        case CL_INVALID_ARG_INDEX:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_ARG_INDEX: arg_index is not a valid argument index.\n");
            break;
        case CL_INVALID_ARG_VALUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_ARG_VALUE: arg_value specified is not a valid value or the argument is an image declared with the read_only qualifier and arg_value refers to an image object created with cl_mem_flags of CL_MEM_WRITE_ONLY or if the image argument is declared with the write_only qualifier and arg_value refers to an image object created with cl_mem_flags of CL_MEM_READ_ONLY.\n");
            break;
        case CL_INVALID_MEM_OBJECT:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_MEM_OBJECT: an argument declared to be a memory object when the specified arg_value is not a valid memory object.\n");
            break;
        case CL_INVALID_SAMPLER:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_SAMPLER: an argument declared to be of type sampler_t when the specified arg_value is not a valid sampler object.\n");
            break;
        case CL_INVALID_DEVICE_QUEUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_DEVICE_QUEUE: an argument declared to be of type queue_t when the specified arg_value is not a valid device queue object. This error code is missing before version 2.0.\n");
            break;
        case CL_INVALID_ARG_SIZE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_ARG_SIZE: arg_size does not match the size of the data type for an argument that is not a memory object or if the argument is a memory object and arg_size != sizeof(cl_mem) or if arg_size is zero and the argument is declared with the local qualifier or if the argument is a sampler and arg_size != sizeof(cl_sampler).\n");
            break;
        case CL_MAX_SIZE_RESTRICTION_EXCEEDED:
            LOG(LOG_TIME | LOG_FUNC, "CL_MAX_SIZE_RESTRICTION_EXCEEDED: the size in bytes of the memory object (if the argument is a memory object) or arg_size (if the argument is declared with local qualifier) exceeds a language- specified maximum size restriction for this argument, such as the MaxByteOffset SPIR-V decoration. This error code is missing before version 2.2.\n");
            break;
        case CL_OUT_OF_RESOURCES:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_RESOURCES: there is a failure to allocate resources required by the OpenCL implementation on the device.\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
            break;
        }
    }
}

static void enq_error(cl_int ret)
{
    if (ret != CL_SUCCESS)
    {
        switch (ret)
        {

        case CL_INVALID_PROGRAM_EXECUTABLE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_PROGRAM_EXECUTABLE: there is no successfully built program executable available for device associated with command_queue.\n");
            break;
        case CL_INVALID_COMMAND_QUEUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_COMMAND_QUEUE: command_queue is not a valid host command-queue.\n");
            break;
        case CL_INVALID_KERNEL:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_KERNEL: kernel is not a valid kernel object.\n");
            break;
        case CL_INVALID_CONTEXT:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_CONTEXT: context associated with command_queue and kernel are not the same or if the context associated with command_queue and events in event_wait_list are not the same.\n");
            break;
        case CL_INVALID_KERNEL_ARGS:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_KERNEL_ARGS: the kernel argument values have not been specified.\n");
            break;
        case CL_INVALID_WORK_DIMENSION:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_WORK_DIMENSION: work_dim is not a valid value (i.e. a value between 1 and CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS).\n");
            break;
        case CL_INVALID_GLOBAL_WORK_SIZE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_GLOBAL_WORK_SIZE: global_work_size is NULL or if any of the values specified in global_work_size[0], …​ global_work_size[work_dim - 1] are 0. Returning this error code under these circumstances is deprecated by version 2.1 or the work-group size must be uniform and the local_work_size is not NULL, is not equal to the required work-group size specified in the kernel source, or the global_work_size is not evenly divisible by the local_work_size or any of the values specified in global_work_size[0], …​ global_work_size[work_dim - 1] exceed the maximum value representable by size_t on the device on which the kernel-instance will be enqueued.\n");
            break;
        case CL_INVALID_GLOBAL_OFFSET:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_GLOBAL_OFFSET: the value specified in global_work_size + the corresponding values in global_work_offset for any dimensions is greater than the maximum value representable by size t on the device on which the kernel-instance will be enqueued, or if global_work_offset is non-NULL before version 1.1.\n");
            break;
        case CL_INVALID_WORK_GROUP_SIZE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_WORK_GROUP_SIZE: local_work_size is specified and does not match the required work-group size for kernel in the program source or local_work_size is specified and is not consistent with the required number of sub-groups for kernel in the program source or local_work_size is specified and the total number of work-items in the work-group computed as local_work_size[0] …​ local_work_size[work_dim - 1] is greater than the value specified by CL_KERNEL_WORK_GROUP_SIZE in the Kernel Object Device Queries table.\n");
            break;
        case CL_INVALID_WORK_ITEM_SIZE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_WORK_ITEM_SIZE: the number of work-items specified in any of local_work_size[0], …​ local_work_size[work_dim - 1] is greater than the corresponding values specified by CL_DEVICE_MAX_WORK_ITEM_SIZES[0], …​, CL_DEVICE_MAX_WORK_ITEM_SIZES[work_dim - 1].\n");
            break;
        case CL_MISALIGNED_SUB_BUFFER_OFFSET:
            LOG(LOG_TIME | LOG_FUNC, "CL_MISALIGNED_SUB_BUFFER_OFFSET: a sub-buffer object is specified as the value for an argument that is a buffer object and the offset specified when the sub-buffer object is created is not aligned to CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with queue. This error code is missing before version 1.1.\n");
            break;
        case CL_INVALID_IMAGE_SIZE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_IMAGE_SIZE: an image object is specified as an argument value and the image dimensions (image width, height, specified or compute row and/or slice pitch) are not supported by device associated with queue.\n");
            break;
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            LOG(LOG_TIME | LOG_FUNC, "CL_IMAGE_FORMAT_NOT_SUPPORTED: an image object is specified as an argument value and the image format (image channel order and data type) is not supported by device associated with queue.\n");
            break;
        case CL_OUT_OF_RESOURCES:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_RESOURCES: there is a failure to queue the execution instance of kernel on the command-queue because of insufficient resources needed to execute the kernel. For example, the explicitly specified local_work_size causes a failure to execute the kernel because of insufficient resources such as registers or local memory. Another example would be the number of read-only image args used in kernel exceed the CL_DEVICE_MAX_READ_IMAGE_ARGS value for device or the number of write-only and read-write image args used in kernel exceed the CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS value for device or the number of samplers used in kernel exceed CL_DEVICE_MAX_SAMPLERS for device or  there is a failure to allocate resources required by the OpenCL implementation on the device.\n");
            break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            LOG(LOG_TIME | LOG_FUNC, "CL_MEM_OBJECT_ALLOCATION_FAILURE: there is a failure to allocate memory for data store associated with image or buffer objects specified as arguments to kernel.\n");
            break;
        case CL_INVALID_EVENT_WAIT_LIST:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_EVENT_WAIT_LIST: event_wait_list is NULL and num_events_in_wait_list > 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events.\n");
            break;
        case CL_INVALID_OPERATION:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_OPERATION: SVM pointers are passed as arguments to a kernel and the device does not support SVM or if system pointers are passed as arguments to a kernel and/or stored inside SVM allocations passed as kernel arguments and the device does not support fine grain system SVM allocations.\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
            break;
        }
    }
}

static void kernel_error(cl_int ret)
{
    if (ret != CL_SUCCESS)
    {
        switch (ret)
        {
        case CL_INVALID_PROGRAM:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_PROGRAM: program is not a valid program object.\n");
            break;
        case CL_INVALID_PROGRAM_EXECUTABLE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_PROGRAM_EXECUTABLE: there is no successfully built executable for program.\n");
            break;
        case CL_INVALID_KERNEL_NAME:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_KERNEL_NAME: kernel_name is not found in program.\n");
            break;
        case CL_INVALID_KERNEL_DEFINITION:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_KERNEL_DEFINITION: the function definition for __kernel function given by kernel_name such as the number of arguments, the argument types are not the same for all devices for which the program executable has been built.\n");
            break;
        case CL_INVALID_VALUE:
            LOG(LOG_TIME | LOG_FUNC, "CL_INVALID_VALUE: kernel_name is NULL.\n");
            break;
        case CL_OUT_OF_RESOURCES:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_RESOURCES: there is a failure to allocate resources required by the OpenCL implementation on the device.\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            LOG(LOG_TIME | LOG_FUNC, "CL_OUT_OF_HOST_MEMORY: there is a failure to allocate resources required by the OpenCL implementation on the host.\n");
            break;
        default:
            LOG(LOG_TIME | LOG_FUNC, "Unknown error\n");
        }
    }
}

void opencl_tick(Particle *p, volatile int *running, int n, double dt)
{
    size_t size_n = (size_t)n;
    cl_int ret;
    cl_int n_param = n;
    cl_double dt_param = dt;
    arg_error(clSetKernelArg(kernel_solve, 0, sizeof(cl_mem), &cl_particles));
    arg_error(clSetKernelArg(kernel_solve, 1, sizeof(cl_double), &dt_param));
    arg_error(clSetKernelArg(kernel_solve, 2, sizeof(cl_int), &n_param));
    arg_error(clSetKernelArg(kernel_update, 0, sizeof(cl_mem), &cl_particles));
    arg_error(clSetKernelArg(kernel_update, 1, sizeof(cl_double), &dt_param));
    arg_error(clSetKernelArg(kernel_update, 2, sizeof(cl_int), &n_param));
    for (int i = 0; i < TIME_STEPS && *running; i++)
    {
        ret = clEnqueueNDRangeKernel(command_queue, kernel_solve, 1, 0, &size_n, &work_dim, 0, NULL, NULL);
        enq_error(ret);
        ret = clEnqueueNDRangeKernel(command_queue, kernel_update, 1, 0, &size_n, &work_dim, 0, NULL, NULL);
        enq_error(ret);
    }
    ret = clEnqueueReadBuffer(command_queue, cl_particles, CL_TRUE, 0, n * sizeof(Particle), p, 0, NULL, NULL);
}

void free_opencl_tick(void)
{
    cl_int ret;
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel_solve);
    ret = clReleaseKernel(kernel_update);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(cl_particles);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
}