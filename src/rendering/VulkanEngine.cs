using System.Runtime.InteropServices;
using Silk.NET.Core;
using Silk.NET.Core.Native;
using Silk.NET.Maths;
using Silk.NET.Vulkan;
using Silk.NET.Vulkan.Extensions.KHR;
using Silk.NET.Windowing;

namespace UniversalSim2.Rendering;

public unsafe class VulkanEngine : IDisposable
{
    private readonly Vk _vk;
    private Instance _instance;
    private PhysicalDevice _physicalDevice;
    private Device _device;
    private Queue _graphicsQueue;
    private Queue _presentQueue;
    private SurfaceKHR _surface;
    private KhrSurface _khrSurface = null!;
    private KhrSwapchain _khrSwapchain = null!;
    private SwapchainKHR _swapchain;
    private Image[] _swapchainImages = Array.Empty<Image>();
    private ImageView[] _swapchainImageViews = Array.Empty<ImageView>();
    private Format _swapchainFormat;
    private Extent2D _swapchainExtent;
    private RenderPass _renderPass;
    private PipelineLayout _pipelineLayout;
    private Pipeline _graphicsPipeline;
    private Framebuffer[] _framebuffers = Array.Empty<Framebuffer>();
    private CommandPool _commandPool;
    private CommandBuffer[] _commandBuffers = Array.Empty<CommandBuffer>();
    private Silk.NET.Vulkan.Semaphore _imageAvailableSemaphore;
    private Silk.NET.Vulkan.Semaphore _renderFinishedSemaphore;
    private Fence _inFlightFence;

    private Silk.NET.Vulkan.Buffer _vertexBuffer;
    private DeviceMemory _vertexBufferMemory;
    private Silk.NET.Vulkan.Buffer _indexBuffer;
    private DeviceMemory _indexBufferMemory;
    private Silk.NET.Vulkan.Buffer _uniformBuffer;
    private DeviceMemory _uniformBufferMemory;
    private DescriptorSetLayout _descriptorSetLayout;
    private DescriptorPool _descriptorPool;
    private DescriptorSet _descriptorSet;

    private uint _indexCount;
    private uint _graphicsQueueFamily;
    private uint _presentQueueFamily;

    private readonly IWindow _window;

    public VulkanEngine(IWindow window)
    {
        _window = window;
        _vk = Vk.GetApi();
    }

    public void Initialize()
    {
        CreateInstance();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapchain();
        CreateImageViews();
        CreateRenderPass();
        CreateDescriptorSetLayout();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandPool();
        CreateSyncObjects();
    }

    public void UploadMesh(float[] vertices, uint[] indices)
    {
        _indexCount = (uint)indices.Length;
        CreateVertexBuffer(vertices);
        CreateIndexBuffer(indices);
        CreateUniformBuffer();
        CreateDescriptorPool();
        CreateDescriptorSet();
        CreateCommandBuffers();
    }

    public void UpdateUniforms(Matrix4X4<float> mvp)
    {
        void* data;
        _vk.MapMemory(_device, _uniformBufferMemory, 0, (ulong)sizeof(Matrix4X4<float>), 0, &data);
        Marshal.Copy(MemoryMarshal.AsBytes(new ReadOnlySpan<Matrix4X4<float>>(&mvp, 1)).ToArray(), 0, (nint)data, sizeof(Matrix4X4<float>));
        _vk.UnmapMemory(_device, _uniformBufferMemory);
    }

    public void DrawFrame()
    {
        var fence = _inFlightFence;
        _vk.WaitForFences(_device, 1, &fence, true, ulong.MaxValue);
        _vk.ResetFences(_device, 1, &fence);

        uint imageIndex;
        _khrSwapchain.AcquireNextImage(_device, _swapchain, ulong.MaxValue, _imageAvailableSemaphore, default, &imageIndex);

        var waitSemaphores = stackalloc[] { _imageAvailableSemaphore };
        var waitStages = stackalloc[] { PipelineStageFlags.ColorAttachmentOutputBit };
        var signalSemaphores = stackalloc[] { _renderFinishedSemaphore };
        var cmdBuf = _commandBuffers[imageIndex];

        var submitInfo = new SubmitInfo(StructureType.SubmitInfo)
        {
            WaitSemaphoreCount = 1,
            PWaitSemaphores = waitSemaphores,
            PWaitDstStageMask = waitStages,
            CommandBufferCount = 1,
            PCommandBuffers = &cmdBuf,
            SignalSemaphoreCount = 1,
            PSignalSemaphores = signalSemaphores
        };

        _vk.QueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFence);

        var swapchains = stackalloc[] { _swapchain };
        var presentInfo = new PresentInfoKHR(StructureType.PresentInfoKhr)
        {
            WaitSemaphoreCount = 1,
            PWaitSemaphores = signalSemaphores,
            SwapchainCount = 1,
            PSwapchains = swapchains,
            PImageIndices = &imageIndex
        };

        _khrSwapchain.QueuePresent(_presentQueue, &presentInfo);
    }

    private void CreateInstance()
    {
        var appInfo = new ApplicationInfo
        {
            SType = StructureType.ApplicationInfo,
            PApplicationName = (byte*)Marshal.StringToHGlobalAnsi("Universal Sim 2"),
            ApplicationVersion = new Version32(0, 0, 2),
            PEngineName = (byte*)Marshal.StringToHGlobalAnsi("UniversalSim2Engine"),
            EngineVersion = new Version32(0, 0, 2),
            ApiVersion = Vk.Version12
        };

        var extensions = _window.VkSurface!.GetRequiredExtensions(out var extCount);

        var createInfo = new InstanceCreateInfo
        {
            SType = StructureType.InstanceCreateInfo,
            PApplicationInfo = &appInfo,
            EnabledExtensionCount = extCount,
            PpEnabledExtensionNames = extensions,
            EnabledLayerCount = 0
        };

        fixed (Instance* inst = &_instance)
        {
            if (_vk.CreateInstance(&createInfo, null, inst) != Result.Success)
                throw new Exception("Failed to create Vulkan instance");
        }

        Marshal.FreeHGlobal((nint)appInfo.PApplicationName);
        Marshal.FreeHGlobal((nint)appInfo.PEngineName);
    }

    private void CreateSurface()
    {
        if (!_vk.TryGetInstanceExtension(_instance, out _khrSurface))
            throw new Exception("KHR_surface extension not available");

        _surface = _window.VkSurface!.Create<AllocationCallbacks>(_instance.ToHandle(), null).ToSurface();
    }

    private void PickPhysicalDevice()
    {
        uint deviceCount = 0;
        _vk.EnumeratePhysicalDevices(_instance, &deviceCount, null);
        if (deviceCount == 0) throw new Exception("No Vulkan-capable GPU found");

        var devices = stackalloc PhysicalDevice[(int)deviceCount];
        _vk.EnumeratePhysicalDevices(_instance, &deviceCount, devices);
        _physicalDevice = devices[0]; // Pick first available GPU

        FindQueueFamilies();
    }

    private void FindQueueFamilies()
    {
        uint queueFamilyCount = 0;
        _vk.GetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, null);
        var queueFamilies = stackalloc QueueFamilyProperties[(int)queueFamilyCount];
        _vk.GetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilies);

        bool foundGraphics = false, foundPresent = false;
        for (uint i = 0; i < queueFamilyCount; i++)
        {
            if (queueFamilies[i].QueueFlags.HasFlag(QueueFlags.GraphicsBit))
            {
                _graphicsQueueFamily = i;
                foundGraphics = true;
            }

            Bool32 presentSupport = false;
            _khrSurface.GetPhysicalDeviceSurfaceSupport(_physicalDevice, i, _surface, &presentSupport);
            if (presentSupport)
            {
                _presentQueueFamily = i;
                foundPresent = true;
            }

            if (foundGraphics && foundPresent) break;
        }

        if (!foundGraphics || !foundPresent)
            throw new Exception("Could not find suitable queue families");
    }

    private void CreateLogicalDevice()
    {
        var uniqueFamilies = _graphicsQueueFamily == _presentQueueFamily
            ? new[] { _graphicsQueueFamily }
            : new[] { _graphicsQueueFamily, _presentQueueFamily };

        var queueCreateInfos = stackalloc DeviceQueueCreateInfo[uniqueFamilies.Length];
        float queuePriority = 1.0f;

        for (int i = 0; i < uniqueFamilies.Length; i++)
        {
            queueCreateInfos[i] = new DeviceQueueCreateInfo
            {
                SType = StructureType.DeviceQueueCreateInfo,
                QueueFamilyIndex = uniqueFamilies[i],
                QueueCount = 1,
                PQueuePriorities = &queuePriority
            };
        }

        var deviceFeatures = new PhysicalDeviceFeatures();
        var swapchainExt = (byte*)Marshal.StringToHGlobalAnsi(KhrSwapchain.ExtensionName);

        var deviceCreateInfo = new DeviceCreateInfo
        {
            SType = StructureType.DeviceCreateInfo,
            QueueCreateInfoCount = (uint)uniqueFamilies.Length,
            PQueueCreateInfos = queueCreateInfos,
            PEnabledFeatures = &deviceFeatures,
            EnabledExtensionCount = 1,
            PpEnabledExtensionNames = &swapchainExt
        };

        fixed (Device* dev = &_device)
        {
            if (_vk.CreateDevice(_physicalDevice, &deviceCreateInfo, null, dev) != Result.Success)
                throw new Exception("Failed to create logical device");
        }

        Marshal.FreeHGlobal((nint)swapchainExt);

        fixed (Queue* q = &_graphicsQueue)
            _vk.GetDeviceQueue(_device, _graphicsQueueFamily, 0, q);
        fixed (Queue* q = &_presentQueue)
            _vk.GetDeviceQueue(_device, _presentQueueFamily, 0, q);

        if (!_vk.TryGetDeviceExtension(_instance, _device, out _khrSwapchain))
            throw new Exception("Failed to get swapchain extension");
    }

    private void CreateSwapchain()
    {
        SurfaceCapabilitiesKHR capabilities;
        _khrSurface.GetPhysicalDeviceSurfaceCapabilities(_physicalDevice, _surface, &capabilities);

        uint formatCount;
        _khrSurface.GetPhysicalDeviceSurfaceFormats(_physicalDevice, _surface, &formatCount, null);
        var formats = stackalloc SurfaceFormatKHR[(int)formatCount];
        _khrSurface.GetPhysicalDeviceSurfaceFormats(_physicalDevice, _surface, &formatCount, formats);

        // Pick B8G8R8A8_SRGB if available, otherwise first
        var surfaceFormat = formats[0];
        for (int i = 0; i < formatCount; i++)
        {
            if (formats[i].Format == Format.B8G8R8A8Srgb && formats[i].ColorSpace == ColorSpaceKHR.SpaceSrgbNonlinearKhr)
            {
                surfaceFormat = formats[i];
                break;
            }
        }

        _swapchainFormat = surfaceFormat.Format;

        // Pick mailbox if available, otherwise FIFO
        uint presentModeCount;
        _khrSurface.GetPhysicalDeviceSurfacePresentModes(_physicalDevice, _surface, &presentModeCount, null);
        var presentModes = stackalloc PresentModeKHR[(int)presentModeCount];
        _khrSurface.GetPhysicalDeviceSurfacePresentModes(_physicalDevice, _surface, &presentModeCount, presentModes);

        var presentMode = PresentModeKHR.FifoKhr;
        for (int i = 0; i < presentModeCount; i++)
        {
            if (presentModes[i] == PresentModeKHR.MailboxKhr)
            {
                presentMode = PresentModeKHR.MailboxKhr;
                break;
            }
        }

        _swapchainExtent = capabilities.CurrentExtent.Width != uint.MaxValue
            ? capabilities.CurrentExtent
            : new Extent2D((uint)_window.Size.X, (uint)_window.Size.Y);

        uint imageCount = capabilities.MinImageCount + 1;
        if (capabilities.MaxImageCount > 0 && imageCount > capabilities.MaxImageCount)
            imageCount = capabilities.MaxImageCount;

        var createInfo = new SwapchainCreateInfoKHR
        {
            SType = StructureType.SwapchainCreateInfoKhr,
            Surface = _surface,
            MinImageCount = imageCount,
            ImageFormat = surfaceFormat.Format,
            ImageColorSpace = surfaceFormat.ColorSpace,
            ImageExtent = _swapchainExtent,
            ImageArrayLayers = 1,
            ImageUsage = ImageUsageFlags.ColorAttachmentBit,
            PreTransform = capabilities.CurrentTransform,
            CompositeAlpha = CompositeAlphaFlagsKHR.OpaqueBitKhr,
            PresentMode = presentMode,
            Clipped = true
        };

        if (_graphicsQueueFamily != _presentQueueFamily)
        {
            var queueFamilyIndices = stackalloc uint[] { _graphicsQueueFamily, _presentQueueFamily };
            createInfo.ImageSharingMode = SharingMode.Concurrent;
            createInfo.QueueFamilyIndexCount = 2;
            createInfo.PQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.ImageSharingMode = SharingMode.Exclusive;
        }

        fixed (SwapchainKHR* sc = &_swapchain)
        {
            if (_khrSwapchain.CreateSwapchain(_device, &createInfo, null, sc) != Result.Success)
                throw new Exception("Failed to create swapchain");
        }

        _khrSwapchain.GetSwapchainImages(_device, _swapchain, &imageCount, null);
        _swapchainImages = new Image[imageCount];
        fixed (Image* img = _swapchainImages)
            _khrSwapchain.GetSwapchainImages(_device, _swapchain, &imageCount, img);
    }

    private void CreateImageViews()
    {
        _swapchainImageViews = new ImageView[_swapchainImages.Length];
        for (int i = 0; i < _swapchainImages.Length; i++)
        {
            var createInfo = new ImageViewCreateInfo
            {
                SType = StructureType.ImageViewCreateInfo,
                Image = _swapchainImages[i],
                ViewType = ImageViewType.Type2D,
                Format = _swapchainFormat,
                Components = new ComponentMapping
                {
                    R = ComponentSwizzle.Identity,
                    G = ComponentSwizzle.Identity,
                    B = ComponentSwizzle.Identity,
                    A = ComponentSwizzle.Identity
                },
                SubresourceRange = new ImageSubresourceRange
                {
                    AspectMask = ImageAspectFlags.ColorBit,
                    BaseMipLevel = 0,
                    LevelCount = 1,
                    BaseArrayLayer = 0,
                    LayerCount = 1
                }
            };

            fixed (ImageView* iv = &_swapchainImageViews[i])
            {
                if (_vk.CreateImageView(_device, &createInfo, null, iv) != Result.Success)
                    throw new Exception("Failed to create image view");
            }
        }
    }

    private void CreateRenderPass()
    {
        var colorAttachment = new AttachmentDescription
        {
            Format = _swapchainFormat,
            Samples = SampleCountFlags.Count1Bit,
            LoadOp = AttachmentLoadOp.Clear,
            StoreOp = AttachmentStoreOp.Store,
            StencilLoadOp = AttachmentLoadOp.DontCare,
            StencilStoreOp = AttachmentStoreOp.DontCare,
            InitialLayout = ImageLayout.Undefined,
            FinalLayout = ImageLayout.PresentSrcKhr
        };

        var colorRef = new AttachmentReference { Attachment = 0, Layout = ImageLayout.ColorAttachmentOptimal };

        var subpass = new SubpassDescription
        {
            PipelineBindPoint = PipelineBindPoint.Graphics,
            ColorAttachmentCount = 1,
            PColorAttachments = &colorRef
        };

        var dependency = new SubpassDependency
        {
            SrcSubpass = Vk.SubpassExternal,
            DstSubpass = 0,
            SrcStageMask = PipelineStageFlags.ColorAttachmentOutputBit,
            SrcAccessMask = 0,
            DstStageMask = PipelineStageFlags.ColorAttachmentOutputBit,
            DstAccessMask = AccessFlags.ColorAttachmentWriteBit
        };

        var renderPassInfo = new RenderPassCreateInfo
        {
            SType = StructureType.RenderPassCreateInfo,
            AttachmentCount = 1,
            PAttachments = &colorAttachment,
            SubpassCount = 1,
            PSubpasses = &subpass,
            DependencyCount = 1,
            PDependencies = &dependency
        };

        fixed (RenderPass* rp = &_renderPass)
        {
            if (_vk.CreateRenderPass(_device, &renderPassInfo, null, rp) != Result.Success)
                throw new Exception("Failed to create render pass");
        }
    }

    private void CreateDescriptorSetLayout()
    {
        var binding = new DescriptorSetLayoutBinding
        {
            Binding = 0,
            DescriptorType = DescriptorType.UniformBuffer,
            DescriptorCount = 1,
            StageFlags = ShaderStageFlags.VertexBit
        };

        var layoutInfo = new DescriptorSetLayoutCreateInfo
        {
            SType = StructureType.DescriptorSetLayoutCreateInfo,
            BindingCount = 1,
            PBindings = &binding
        };

        fixed (DescriptorSetLayout* layout = &_descriptorSetLayout)
        {
            if (_vk.CreateDescriptorSetLayout(_device, &layoutInfo, null, layout) != Result.Success)
                throw new Exception("Failed to create descriptor set layout");
        }
    }

    private void CreateGraphicsPipeline()
    {
        var vertCode = ShaderHelper.GetVertexShaderSpirV();
        var fragCode = ShaderHelper.GetFragmentShaderSpirV();

        var vertModule = CreateShaderModule(vertCode);
        var fragModule = CreateShaderModule(fragCode);

        var entryPoint = (byte*)Marshal.StringToHGlobalAnsi("main");

        var vertStage = new PipelineShaderStageCreateInfo
        {
            SType = StructureType.PipelineShaderStageCreateInfo,
            Stage = ShaderStageFlags.VertexBit,
            Module = vertModule,
            PName = entryPoint
        };

        var fragStage = new PipelineShaderStageCreateInfo
        {
            SType = StructureType.PipelineShaderStageCreateInfo,
            Stage = ShaderStageFlags.FragmentBit,
            Module = fragModule,
            PName = entryPoint
        };

        var shaderStages = stackalloc[] { vertStage, fragStage };

        // Vertex input: position (vec3) + normal (vec3) = 6 floats, 24 bytes
        var bindingDesc = new VertexInputBindingDescription
        {
            Binding = 0,
            Stride = 6 * sizeof(float),
            InputRate = VertexInputRate.Vertex
        };

        var attrDescs = stackalloc VertexInputAttributeDescription[]
        {
            new() { Binding = 0, Location = 0, Format = Format.R32G32B32Sfloat, Offset = 0 },
            new() { Binding = 0, Location = 1, Format = Format.R32G32B32Sfloat, Offset = 3 * sizeof(float) }
        };

        var vertexInputInfo = new PipelineVertexInputStateCreateInfo
        {
            SType = StructureType.PipelineVertexInputStateCreateInfo,
            VertexBindingDescriptionCount = 1,
            PVertexBindingDescriptions = &bindingDesc,
            VertexAttributeDescriptionCount = 2,
            PVertexAttributeDescriptions = attrDescs
        };

        var inputAssembly = new PipelineInputAssemblyStateCreateInfo
        {
            SType = StructureType.PipelineInputAssemblyStateCreateInfo,
            Topology = PrimitiveTopology.TriangleList,
            PrimitiveRestartEnable = false
        };

        var viewport = new Viewport
        {
            X = 0, Y = 0,
            Width = _swapchainExtent.Width,
            Height = _swapchainExtent.Height,
            MinDepth = 0, MaxDepth = 1
        };

        var scissor = new Rect2D { Offset = default, Extent = _swapchainExtent };

        var viewportState = new PipelineViewportStateCreateInfo
        {
            SType = StructureType.PipelineViewportStateCreateInfo,
            ViewportCount = 1,
            PViewports = &viewport,
            ScissorCount = 1,
            PScissors = &scissor
        };

        var rasterizer = new PipelineRasterizationStateCreateInfo
        {
            SType = StructureType.PipelineRasterizationStateCreateInfo,
            DepthClampEnable = false,
            RasterizerDiscardEnable = false,
            PolygonMode = PolygonMode.Fill,
            LineWidth = 1.0f,
            CullMode = CullModeFlags.BackBit,
            FrontFace = FrontFace.CounterClockwise,
            DepthBiasEnable = false
        };

        var multisampling = new PipelineMultisampleStateCreateInfo
        {
            SType = StructureType.PipelineMultisampleStateCreateInfo,
            SampleShadingEnable = false,
            RasterizationSamples = SampleCountFlags.Count1Bit
        };

        var colorBlendAttachment = new PipelineColorBlendAttachmentState
        {
            ColorWriteMask = ColorComponentFlags.RBit | ColorComponentFlags.GBit | ColorComponentFlags.BBit | ColorComponentFlags.ABit,
            BlendEnable = false
        };

        var colorBlending = new PipelineColorBlendStateCreateInfo
        {
            SType = StructureType.PipelineColorBlendStateCreateInfo,
            LogicOpEnable = false,
            AttachmentCount = 1,
            PAttachments = &colorBlendAttachment
        };

        fixed (DescriptorSetLayout* dsl = &_descriptorSetLayout)
        {
            var pipelineLayoutInfo = new PipelineLayoutCreateInfo
            {
                SType = StructureType.PipelineLayoutCreateInfo,
                SetLayoutCount = 1,
                PSetLayouts = dsl
            };

            fixed (PipelineLayout* pl = &_pipelineLayout)
            {
                if (_vk.CreatePipelineLayout(_device, &pipelineLayoutInfo, null, pl) != Result.Success)
                    throw new Exception("Failed to create pipeline layout");
            }
        }

        var pipelineInfo = new GraphicsPipelineCreateInfo
        {
            SType = StructureType.GraphicsPipelineCreateInfo,
            StageCount = 2,
            PStages = shaderStages,
            PVertexInputState = &vertexInputInfo,
            PInputAssemblyState = &inputAssembly,
            PViewportState = &viewportState,
            PRasterizationState = &rasterizer,
            PMultisampleState = &multisampling,
            PColorBlendState = &colorBlending,
            Layout = _pipelineLayout,
            RenderPass = _renderPass,
            Subpass = 0
        };

        fixed (Pipeline* gp = &_graphicsPipeline)
        {
            if (_vk.CreateGraphicsPipelines(_device, default, 1, &pipelineInfo, null, gp) != Result.Success)
                throw new Exception("Failed to create graphics pipeline");
        }

        Marshal.FreeHGlobal((nint)entryPoint);
        _vk.DestroyShaderModule(_device, vertModule, null);
        _vk.DestroyShaderModule(_device, fragModule, null);
    }

    private ShaderModule CreateShaderModule(byte[] code)
    {
        fixed (byte* codePtr = code)
        {
            var createInfo = new ShaderModuleCreateInfo
            {
                SType = StructureType.ShaderModuleCreateInfo,
                CodeSize = (nuint)code.Length,
                PCode = (uint*)codePtr
            };

            ShaderModule module;
            if (_vk.CreateShaderModule(_device, &createInfo, null, &module) != Result.Success)
                throw new Exception("Failed to create shader module");
            return module;
        }
    }

    private void CreateFramebuffers()
    {
        _framebuffers = new Framebuffer[_swapchainImageViews.Length];
        for (int i = 0; i < _swapchainImageViews.Length; i++)
        {
            var attachment = _swapchainImageViews[i];
            var fbInfo = new FramebufferCreateInfo
            {
                SType = StructureType.FramebufferCreateInfo,
                RenderPass = _renderPass,
                AttachmentCount = 1,
                PAttachments = &attachment,
                Width = _swapchainExtent.Width,
                Height = _swapchainExtent.Height,
                Layers = 1
            };

            fixed (Framebuffer* fb = &_framebuffers[i])
            {
                if (_vk.CreateFramebuffer(_device, &fbInfo, null, fb) != Result.Success)
                    throw new Exception("Failed to create framebuffer");
            }
        }
    }

    private void CreateCommandPool()
    {
        var poolInfo = new CommandPoolCreateInfo
        {
            SType = StructureType.CommandPoolCreateInfo,
            QueueFamilyIndex = _graphicsQueueFamily,
            Flags = CommandPoolCreateFlags.ResetCommandBufferBit
        };

        fixed (CommandPool* cp = &_commandPool)
        {
            if (_vk.CreateCommandPool(_device, &poolInfo, null, cp) != Result.Success)
                throw new Exception("Failed to create command pool");
        }
    }

    private void CreateSyncObjects()
    {
        var semaphoreInfo = new SemaphoreCreateInfo { SType = StructureType.SemaphoreCreateInfo };
        var fenceInfo = new FenceCreateInfo { SType = StructureType.FenceCreateInfo, Flags = FenceCreateFlags.SignaledBit };

        fixed (Silk.NET.Vulkan.Semaphore* ia = &_imageAvailableSemaphore)
        fixed (Silk.NET.Vulkan.Semaphore* rf = &_renderFinishedSemaphore)
        fixed (Fence* f = &_inFlightFence)
        {
            if (_vk.CreateSemaphore(_device, &semaphoreInfo, null, ia) != Result.Success ||
                _vk.CreateSemaphore(_device, &semaphoreInfo, null, rf) != Result.Success ||
                _vk.CreateFence(_device, &fenceInfo, null, f) != Result.Success)
                throw new Exception("Failed to create sync objects");
        }
    }

    private void CreateVertexBuffer(float[] vertices)
    {
        ulong bufferSize = (ulong)(vertices.Length * sizeof(float));
        CreateBuffer(bufferSize, BufferUsageFlags.VertexBufferBit,
            MemoryPropertyFlags.HostVisibleBit | MemoryPropertyFlags.HostCoherentBit,
            out _vertexBuffer, out _vertexBufferMemory);

        void* data;
        _vk.MapMemory(_device, _vertexBufferMemory, 0, bufferSize, 0, &data);
        fixed (float* src = vertices)
            System.Buffer.MemoryCopy(src, data, bufferSize, bufferSize);
        _vk.UnmapMemory(_device, _vertexBufferMemory);
    }

    private void CreateIndexBuffer(uint[] indices)
    {
        ulong bufferSize = (ulong)(indices.Length * sizeof(uint));
        CreateBuffer(bufferSize, BufferUsageFlags.IndexBufferBit,
            MemoryPropertyFlags.HostVisibleBit | MemoryPropertyFlags.HostCoherentBit,
            out _indexBuffer, out _indexBufferMemory);

        void* data;
        _vk.MapMemory(_device, _indexBufferMemory, 0, bufferSize, 0, &data);
        fixed (uint* src = indices)
            System.Buffer.MemoryCopy(src, data, bufferSize, bufferSize);
        _vk.UnmapMemory(_device, _indexBufferMemory);
    }

    private void CreateUniformBuffer()
    {
        ulong bufferSize = (ulong)sizeof(Matrix4X4<float>);
        CreateBuffer(bufferSize, BufferUsageFlags.UniformBufferBit,
            MemoryPropertyFlags.HostVisibleBit | MemoryPropertyFlags.HostCoherentBit,
            out _uniformBuffer, out _uniformBufferMemory);
    }

    private void CreateBuffer(ulong size, BufferUsageFlags usage, MemoryPropertyFlags properties,
        out Silk.NET.Vulkan.Buffer buffer, out DeviceMemory memory)
    {
        var bufferInfo = new BufferCreateInfo
        {
            SType = StructureType.BufferCreateInfo,
            Size = size,
            Usage = usage,
            SharingMode = SharingMode.Exclusive
        };

        fixed (Silk.NET.Vulkan.Buffer* buf = &buffer)
        {
            if (_vk.CreateBuffer(_device, &bufferInfo, null, buf) != Result.Success)
                throw new Exception("Failed to create buffer");
        }

        MemoryRequirements memReqs;
        _vk.GetBufferMemoryRequirements(_device, buffer, &memReqs);

        var allocInfo = new MemoryAllocateInfo
        {
            SType = StructureType.MemoryAllocateInfo,
            AllocationSize = memReqs.Size,
            MemoryTypeIndex = FindMemoryType(memReqs.MemoryTypeBits, properties)
        };

        fixed (DeviceMemory* mem = &memory)
        {
            if (_vk.AllocateMemory(_device, &allocInfo, null, mem) != Result.Success)
                throw new Exception("Failed to allocate buffer memory");
        }

        _vk.BindBufferMemory(_device, buffer, memory, 0);
    }

    private uint FindMemoryType(uint typeFilter, MemoryPropertyFlags properties)
    {
        PhysicalDeviceMemoryProperties memProperties;
        _vk.GetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

        for (uint i = 0; i < memProperties.MemoryTypeCount; i++)
        {
            if ((typeFilter & (1u << (int)i)) != 0 &&
                (memProperties.MemoryTypes[(int)i].PropertyFlags & properties) == properties)
                return i;
        }

        throw new Exception("Failed to find suitable memory type");
    }

    private void CreateDescriptorPool()
    {
        var poolSize = new DescriptorPoolSize
        {
            Type = DescriptorType.UniformBuffer,
            DescriptorCount = 1
        };

        var poolInfo = new DescriptorPoolCreateInfo
        {
            SType = StructureType.DescriptorPoolCreateInfo,
            PoolSizeCount = 1,
            PPoolSizes = &poolSize,
            MaxSets = 1
        };

        fixed (DescriptorPool* dp = &_descriptorPool)
        {
            if (_vk.CreateDescriptorPool(_device, &poolInfo, null, dp) != Result.Success)
                throw new Exception("Failed to create descriptor pool");
        }
    }

    private void CreateDescriptorSet()
    {
        fixed (DescriptorSetLayout* dsl = &_descriptorSetLayout)
        {
            var allocInfo = new DescriptorSetAllocateInfo
            {
                SType = StructureType.DescriptorSetAllocateInfo,
                DescriptorPool = _descriptorPool,
                DescriptorSetCount = 1,
                PSetLayouts = dsl
            };

            fixed (DescriptorSet* ds = &_descriptorSet)
            {
                if (_vk.AllocateDescriptorSets(_device, &allocInfo, ds) != Result.Success)
                    throw new Exception("Failed to allocate descriptor set");
            }
        }

        var bufferInfo = new DescriptorBufferInfo
        {
            Buffer = _uniformBuffer,
            Offset = 0,
            Range = (ulong)sizeof(Matrix4X4<float>)
        };

        var descriptorWrite = new WriteDescriptorSet
        {
            SType = StructureType.WriteDescriptorSet,
            DstSet = _descriptorSet,
            DstBinding = 0,
            DstArrayElement = 0,
            DescriptorType = DescriptorType.UniformBuffer,
            DescriptorCount = 1,
            PBufferInfo = &bufferInfo
        };

        _vk.UpdateDescriptorSets(_device, 1, &descriptorWrite, 0, null);
    }

    private void CreateCommandBuffers()
    {
        _commandBuffers = new CommandBuffer[_framebuffers.Length];

        var allocInfo = new CommandBufferAllocateInfo
        {
            SType = StructureType.CommandBufferAllocateInfo,
            CommandPool = _commandPool,
            Level = CommandBufferLevel.Primary,
            CommandBufferCount = (uint)_commandBuffers.Length
        };

        fixed (CommandBuffer* cb = _commandBuffers)
        {
            if (_vk.AllocateCommandBuffers(_device, &allocInfo, cb) != Result.Success)
                throw new Exception("Failed to allocate command buffers");
        }

        for (int i = 0; i < _commandBuffers.Length; i++)
        {
            var beginInfo = new CommandBufferBeginInfo { SType = StructureType.CommandBufferBeginInfo };
            _vk.BeginCommandBuffer(_commandBuffers[i], &beginInfo);

            var clearColor = new ClearValue { Color = new ClearColorValue(0f, 0f, 0f, 1f) };

            var renderPassBegin = new RenderPassBeginInfo
            {
                SType = StructureType.RenderPassBeginInfo,
                RenderPass = _renderPass,
                Framebuffer = _framebuffers[i],
                RenderArea = new Rect2D { Offset = default, Extent = _swapchainExtent },
                ClearValueCount = 1,
                PClearValues = &clearColor
            };

            _vk.CmdBeginRenderPass(_commandBuffers[i], &renderPassBegin, SubpassContents.Inline);
            _vk.CmdBindPipeline(_commandBuffers[i], PipelineBindPoint.Graphics, _graphicsPipeline);

            var vertBuf = _vertexBuffer;
            ulong offset = 0;
            _vk.CmdBindVertexBuffers(_commandBuffers[i], 0, 1, &vertBuf, &offset);
            _vk.CmdBindIndexBuffer(_commandBuffers[i], _indexBuffer, 0, IndexType.Uint32);

            fixed (DescriptorSet* ds = &_descriptorSet)
                _vk.CmdBindDescriptorSets(_commandBuffers[i], PipelineBindPoint.Graphics, _pipelineLayout, 0, 1, ds, 0, null);

            _vk.CmdDrawIndexed(_commandBuffers[i], _indexCount, 1, 0, 0, 0);
            _vk.CmdEndRenderPass(_commandBuffers[i]);
            _vk.EndCommandBuffer(_commandBuffers[i]);
        }
    }

    public void WaitIdle()
    {
        _vk.DeviceWaitIdle(_device);
    }

    public void Dispose()
    {
        WaitIdle();

        _vk.DestroyBuffer(_device, _vertexBuffer, null);
        _vk.FreeMemory(_device, _vertexBufferMemory, null);
        _vk.DestroyBuffer(_device, _indexBuffer, null);
        _vk.FreeMemory(_device, _indexBufferMemory, null);
        _vk.DestroyBuffer(_device, _uniformBuffer, null);
        _vk.FreeMemory(_device, _uniformBufferMemory, null);
        _vk.DestroyDescriptorPool(_device, _descriptorPool, null);
        _vk.DestroyDescriptorSetLayout(_device, _descriptorSetLayout, null);
        _vk.DestroySemaphore(_device, _imageAvailableSemaphore, null);
        _vk.DestroySemaphore(_device, _renderFinishedSemaphore, null);
        _vk.DestroyFence(_device, _inFlightFence, null);
        _vk.DestroyCommandPool(_device, _commandPool, null);
        foreach (var fb in _framebuffers) _vk.DestroyFramebuffer(_device, fb, null);
        _vk.DestroyPipeline(_device, _graphicsPipeline, null);
        _vk.DestroyPipelineLayout(_device, _pipelineLayout, null);
        _vk.DestroyRenderPass(_device, _renderPass, null);
        foreach (var iv in _swapchainImageViews) _vk.DestroyImageView(_device, iv, null);
        _khrSwapchain.DestroySwapchain(_device, _swapchain, null);
        _vk.DestroyDevice(_device, null);
        _khrSurface.DestroySurface(_instance, _surface, null);
        _vk.DestroyInstance(_instance, null);
        _vk.Dispose();
    }
}
