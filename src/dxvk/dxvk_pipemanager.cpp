#include "dxvk_device.h"
#include "dxvk_pipemanager.h"
#include "dxvk_state_cache.h"

namespace dxvk {
  
  size_t DxvkPipelineKeyHash::operator () (const DxvkComputePipelineShaders& key) const {
    std::hash<DxvkShader*> hash;
    return hash(key.cs.ptr());
  }
  
  
  size_t DxvkPipelineKeyHash::operator () (const DxvkGraphicsPipelineShaders& key) const {
    DxvkHashState state;
    state.add(getShaderHash(key.vs));
    state.add(getShaderHash(key.tcs));
    state.add(getShaderHash(key.tes));
    state.add(getShaderHash(key.gs));
    state.add(getShaderHash(key.fs));
    return state;
  }


  size_t DxvkPipelineKeyHash::getShaderHash(const Rc<DxvkShader>& shader) {
    return shader != nullptr ? shader->getHash() : 0;
  }
  
  
  bool DxvkPipelineKeyEq::operator () (
    const DxvkComputePipelineShaders& a,
    const DxvkComputePipelineShaders& b) const {
    return a.cs == b.cs;
  }
  
  
  bool DxvkPipelineKeyEq::operator () (
    const DxvkGraphicsPipelineShaders& a,
    const DxvkGraphicsPipelineShaders& b) const {
    return a.vs == b.vs && a.tcs == b.tcs
        && a.tes == b.tes && a.gs == b.gs
        && a.fs == b.fs;
  }
  
  
  DxvkPipelineManager::DxvkPipelineManager(
    const DxvkDevice*         device,
          DxvkRenderPassPool* passManager)
  : m_device    (device),
    m_cache     (new DxvkPipelineCache(device->vkd())) {
    std::string useStateCache = env::getEnvVar("DXVK_STATE_CACHE");
    
    if (useStateCache != "0" && device->config().enableStateCache)
      m_stateCache = new DxvkStateCache(device, this, passManager);
  }
  
  
  DxvkPipelineManager::~DxvkPipelineManager() {
    
  }
  
  
  DxvkComputePipeline* DxvkPipelineManager::createComputePipeline(
    const DxvkComputePipelineShaders& shaders) {
    if (shaders.cs == nullptr)
      return nullptr;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto pair = m_computePipelines.find(shaders);
    if (pair != m_computePipelines.end())
      return &pair->second;
    
    auto iter = m_computePipelines.emplace(
      std::piecewise_construct,
      std::tuple(shaders),
      std::tuple(this, shaders));
    return &iter.first->second;
  }
  
  
  DxvkGraphicsPipeline* DxvkPipelineManager::createGraphicsPipeline(
    const DxvkGraphicsPipelineShaders& shaders) {
    if (shaders.vs == nullptr)
      return nullptr;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto pair = m_graphicsPipelines.find(shaders);
    if (pair != m_graphicsPipelines.end())
      return &pair->second;
    
    auto iter = m_graphicsPipelines.emplace(
      std::piecewise_construct,
      std::tuple(shaders),
      std::tuple(this, shaders));
    return &iter.first->second;
  }

  
  void DxvkPipelineManager::registerShader(
    const Rc<DxvkShader>&         shader) {
    if (m_stateCache != nullptr)
      m_stateCache->registerShader(shader);
  }


  DxvkPipelineCount DxvkPipelineManager::getPipelineCount() const {
    DxvkPipelineCount result;
    result.numComputePipelines  = m_numComputePipelines.load();
    result.numGraphicsPipelines = m_numGraphicsPipelines.load();
    return result;
  }


  bool DxvkPipelineManager::isCompilingShaders() const {
    return m_stateCache != nullptr
        && m_stateCache->isCompilingShaders();
  }
  
}
