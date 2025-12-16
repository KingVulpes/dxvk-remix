/*
* Copyright (c) 2023-2025, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

#include "rtx_resources.h"
#include "rtx_options.h"

namespace dxvk {

  class RtxAntiLag : public CommonDeviceObject {
  public:
    explicit RtxAntiLag(DxvkDevice* device);
    ~RtxAntiLag();

    /**
      * \brief: Updates Anti-Lag before processing user input. Should be called immediately before 
      * the application processes user input to reduce latency.
      */
    void updateBeforeInput(std::uint64_t frameId) const;
    
    /**
      * \brief: Updates Anti-Lag before presentation. Should be called before vkQueuePresentKHR
      * with matching frame information.
      */
    void updateBeforePresent(std::uint64_t frameId) const;

    /**
      * \brief: Updates the Anti-Lag mode based on current options.
      */
    void updateMode();

    /**
      * \brief: Returns true if Anti-Lag is requested to be enabled. This does not mean Anti-Lag is in use
      * as it may be using the Off mode or was unable to initialize successfully.
      */
    bool antiLagEnabled() const { return m_enabled; }
    
    /**
      * \brief: Returns true if Anti-Lag is enabled and was initialized successfully. Much like the enabled
      * check this does not mean Anti-Lag is in use as it may be using the Off mode.
      */
    bool antiLagInitialized() const { return m_initialized; }

  private:
    // Note: Cached from options determining this state on construction as Anti-Lag currently only has 1
    // chance to be initialized, meaning this state cannot be changed at runtime past the point of construction.
    bool m_enabled = false;
    bool m_initialized = false;
    AntiLagMode m_currentAntiLagMode = AntiLagMode::Off;
    PFN_vkAntiLagUpdateAMD m_vkAntiLagUpdateAMD = nullptr;
  };

}
