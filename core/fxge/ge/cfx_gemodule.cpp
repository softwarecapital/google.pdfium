// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/cfx_gemodule.h"

#include "core/fxcodec/fx_codec.h"
#include "core/fxge/cfx_fontcache.h"
#include "core/fxge/cfx_fontmgr.h"
#include "core/fxge/ge/cfx_folderfontinfo.h"
#include "core/fxge/ge/fx_text_int.h"
#include "third_party/base/ptr_util.h"

namespace {

CFX_GEModule* g_pGEModule = nullptr;

}  // namespace

CFX_GEModule::CFX_GEModule()
    : m_pFontMgr(pdfium::MakeUnique<CFX_FontMgr>()),
      m_pCodecModule(pdfium::MakeUnique<CCodec_ModuleMgr>()),
      m_pPlatformData(nullptr),
      m_pUserFontPaths(nullptr) {}

CFX_GEModule::~CFX_GEModule() {
  DestroyPlatform();
}

// static
CFX_GEModule* CFX_GEModule::Get() {
  if (!g_pGEModule)
    g_pGEModule = new CFX_GEModule();
  return g_pGEModule;
}

// static
void CFX_GEModule::Destroy() {
  ASSERT(g_pGEModule);
  delete g_pGEModule;
  g_pGEModule = nullptr;
}

void CFX_GEModule::Init(const char** userFontPaths) {
  ASSERT(g_pGEModule);
  m_pUserFontPaths = userFontPaths;
  InitPlatform();
  SetTextGamma(2.2f);
}

CFX_FontCache* CFX_GEModule::GetFontCache() {
  if (!m_pFontCache)
    m_pFontCache = pdfium::MakeUnique<CFX_FontCache>();
  return m_pFontCache.get();
}

void CFX_GEModule::SetTextGamma(float gammaValue) {
  gammaValue /= 2.2f;
  for (int i = 0; i < 256; ++i) {
    m_GammaValue[i] = static_cast<uint8_t>(
        FXSYS_pow(static_cast<float>(i) / 255, gammaValue) * 255.0f + 0.5f);
  }
}

const uint8_t* CFX_GEModule::GetTextGammaTable() const {
  return m_GammaValue;
}
