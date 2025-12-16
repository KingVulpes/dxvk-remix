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
#include "rtx_antilag.h"
#include "dxvk_device.h"

#include <cassert>

namespace dxvk {

  RtxAntiLag::RtxAntiLag(DxvkDevice* device) : CommonDeviceObject(device) {
    // Determine Anti-Lag enablement
    m_enabled = RtxOptions::isAntiLagEnabled();

    // Note: Skip initializing Anti-Lag if it is globally disabled at the time of construction.
    if (!antiLagEnabled()) {
      return;
    }

    // Check if the extension is available
    const auto& extensions = device->extensions();
    if (!extensions.amdAntiLag) {
      Logger::warn("AMD Anti-Lag extension not available on this device.");
      return;
    }

    // Get the function pointer for vkAntiLagUpdateAMD
    Rc<vk::DeviceFn> vkd = m_device->vkd();
    m_vkAntiLagUpdateAMD = reinterpret_cast<PFN_vkAntiLagUpdateAMD>(
      vkd->sym("vkAntiLagUpdateAMD"));

    if (m_vkAntiLagUpdateAMD == nullptr) {
      Logger::err("Failed to get vkAntiLagUpdateAMD function pointer.");
      return;
    }

    updateMode();

    // Mark Anti-Lag as initialized
    m_initialized = true;

    Logger::info("AMD Anti-Lag initialized successfully.");
  }

  RtxAntiLag::~RtxAntiLag() {
    // Early out if Anti-Lag was not initialized
    if (!antiLagInitialized()) {
      return;
    }

    // Note: No explicit cleanup needed for AMD Anti-Lag, just set mode to off
    if (m_vkAntiLagUpdateAMD != nullptr && m_currentAntiLagMode != AntiLagMode::Off) {
      VkAntiLagDataAMD antiLagData = {};
      antiLagData.sType = VK_STRUCTURE_TYPE_ANTI_LAG_DATA_AMD;
      antiLagData.mode = VK_ANTI_LAG_MODE_OFF_AMD;
      antiLagData.maxFPS = 0;
      antiLagData.pPresentationInfo = nullptr;

      m_vkAntiLagUpdateAMD(m_device->vkd()->device(), &antiLagData);
    }
  }

  void RtxAntiLag::updateBeforeInput(std::uint64_t frameId) const {
    // Early out if Anti-Lag was not initialized
    if (!antiLagInitialized() || m_vkAntiLagUpdateAMD == nullptr) {
      return;
    }

    // Skip if mode is off
    if (m_currentAntiLagMode == AntiLagMode::Off) {
      return;
    }

    // Update Anti-Lag before input processing
    VkAntiLagDataAMD antiLagData = {};
    antiLagData.sType = VK_STRUCTURE_TYPE_ANTI_LAG_DATA_AMD;
    antiLagData.mode = VK_ANTI_LAG_MODE_ON_AMD;
    antiLagData.maxFPS = 0; // No FPS cap
    antiLagData.pPresentationInfo = nullptr;

    m_vkAntiLagUpdateAMD(m_device->vkd()->device(), &antiLagData);
  }

  void RtxAntiLag::updateBeforePresent(std::uint64_t frameId) const {
    // Early out if Anti-Lag was not initialized
    if (!antiLagInitialized() || m_vkAntiLagUpdateAMD == nullptr) {
      return;
    }

    // Skip if mode is off
    if (m_currentAntiLagMode == AntiLagMode::Off) {
      return;
    }

    // Update Anti-Lag before presentation
    VkAntiLagPresentationInfoAMD presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_ANTI_LAG_PRESENTATION_INFO_AMD;
    presentInfo.stage = VK_ANTI_LAG_STAGE_PRESENT_AMD;
    presentInfo.frameIndex = frameId;

    VkAntiLagDataAMD antiLagData = {};
    antiLagData.sType = VK_STRUCTURE_TYPE_ANTI_LAG_DATA_AMD;
    antiLagData.mode = VK_ANTI_LAG_MODE_ON_AMD;
    antiLagData.maxFPS = 0; // No FPS cap
    antiLagData.pPresentationInfo = &presentInfo;

    m_vkAntiLagUpdateAMD(m_device->vkd()->device(), &antiLagData);
  }

  void RtxAntiLag::updateMode() {
    if (!antiLagInitialized() || m_vkAntiLagUpdateAMD == nullptr) {
      return;
    }

    // Check the current Anti-Lag Mode
    const auto newMode = RtxOptions::antiLagMode();

    if (newMode == m_currentAntiLagMode) {
      return;
    }

    // Update Anti-Lag mode
    VkAntiLagDataAMD antiLagData = {};
    antiLagData.sType = VK_STRUCTURE_TYPE_ANTI_LAG_DATA_AMD;
    antiLagData.maxFPS = 0; // No FPS cap
    antiLagData.pPresentationInfo = nullptr;

    switch (newMode) {
    case AntiLagMode::Off:
      antiLagData.mode = VK_ANTI_LAG_MODE_OFF_AMD;
      break;
    case AntiLagMode::On:
      antiLagData.mode = VK_ANTI_LAG_MODE_ON_AMD;
      break;
    case AntiLagMode::DriverControl:
      antiLagData.mode = VK_ANTI_LAG_MODE_DRIVER_CONTROL_AMD;
      break;
    }

    m_vkAntiLagUpdateAMD(m_device->vkd()->device(), &antiLagData);

    m_currentAntiLagMode = newMode;

    Logger::info(str::format("AMD Anti-Lag mode changed to ", static_cast<int>(newMode)));
  }

}
