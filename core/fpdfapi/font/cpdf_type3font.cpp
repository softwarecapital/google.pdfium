// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/font/cpdf_type3font.h"

#include <utility>

#include "core/fpdfapi/font/cpdf_type3char.h"
#include "core/fpdfapi/page/cpdf_form.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcrt/fx_system.h"
#include "third_party/base/stl_util.h"

#define FPDF_MAX_TYPE3_FORM_LEVEL 4

CPDF_Type3Font::CPDF_Type3Font()
    : m_pCharProcs(nullptr),
      m_pPageResources(nullptr),
      m_pFontResources(nullptr),
      m_CharLoadingDepth(0) {
  memset(m_CharWidthL, 0, sizeof(m_CharWidthL));
}

CPDF_Type3Font::~CPDF_Type3Font() {}

bool CPDF_Type3Font::IsType3Font() const {
  return true;
}

const CPDF_Type3Font* CPDF_Type3Font::AsType3Font() const {
  return this;
}

CPDF_Type3Font* CPDF_Type3Font::AsType3Font() {
  return this;
}

bool CPDF_Type3Font::Load() {
  m_pFontResources = m_pFontDict->GetDictFor("Resources");
  CPDF_Array* pMatrix = m_pFontDict->GetArrayFor("FontMatrix");
  float xscale = 1.0f;
  float yscale = 1.0f;
  if (pMatrix) {
    m_FontMatrix = pMatrix->GetMatrix();
    xscale = m_FontMatrix.a;
    yscale = m_FontMatrix.d;
  }

  CPDF_Array* pBBox = m_pFontDict->GetArrayFor("FontBBox");
  if (pBBox) {
    m_FontBBox.left =
        static_cast<int32_t>(pBBox->GetNumberAt(0) * xscale * 1000);
    m_FontBBox.bottom =
        static_cast<int32_t>(pBBox->GetNumberAt(1) * yscale * 1000);
    m_FontBBox.right =
        static_cast<int32_t>(pBBox->GetNumberAt(2) * xscale * 1000);
    m_FontBBox.top =
        static_cast<int32_t>(pBBox->GetNumberAt(3) * yscale * 1000);
  }

  int StartChar = m_pFontDict->GetIntegerFor("FirstChar");
  CPDF_Array* pWidthArray = m_pFontDict->GetArrayFor("Widths");
  if (pWidthArray && StartChar >= 0 && StartChar < 256) {
    size_t count = pWidthArray->GetCount();
    if (count > 256)
      count = 256;
    if (StartChar + count > 256)
      count = 256 - StartChar;
    for (size_t i = 0; i < count; i++) {
      m_CharWidthL[StartChar + i] =
          FXSYS_round(pWidthArray->GetNumberAt(i) * xscale * 1000);
    }
  }
  m_pCharProcs = m_pFontDict->GetDictFor("CharProcs");
  CPDF_Object* pEncoding = m_pFontDict->GetDirectObjectFor("Encoding");
  if (pEncoding)
    LoadPDFEncoding(pEncoding, m_BaseEncoding, &m_CharNames, false, false);
  return true;
}

void CPDF_Type3Font::CheckType3FontMetrics() {
  CheckFontMetrics();
}

CPDF_Type3Char* CPDF_Type3Font::LoadChar(uint32_t charcode) {
  if (m_CharLoadingDepth >= FPDF_MAX_TYPE3_FORM_LEVEL)
    return nullptr;

  auto it = m_CacheMap.find(charcode);
  if (it != m_CacheMap.end())
    return it->second.get();

  const char* name = GetAdobeCharName(m_BaseEncoding, m_CharNames, charcode);
  if (!name)
    return nullptr;

  CPDF_Stream* pStream =
      ToStream(m_pCharProcs ? m_pCharProcs->GetDirectObjectFor(name) : nullptr);
  if (!pStream)
    return nullptr;

  auto pNewChar =
      pdfium::MakeUnique<CPDF_Type3Char>(pdfium::MakeUnique<CPDF_Form>(
          m_pDocument.Get(),
          m_pFontResources ? m_pFontResources.Get() : m_pPageResources.Get(),
          pStream, nullptr));

  // This can trigger recursion into this method. The content of |m_CacheMap|
  // can change as a result. Thus after it returns, check the cache again for
  // a cache hit.
  m_CharLoadingDepth++;
  pNewChar->form()->ParseContentWithParams(nullptr, nullptr, pNewChar.get(), 0);
  m_CharLoadingDepth--;
  it = m_CacheMap.find(charcode);
  if (it != m_CacheMap.end())
    return it->second.get();

  pNewChar->Transform(m_FontMatrix);
  m_CacheMap[charcode] = std::move(pNewChar);
  CPDF_Type3Char* pCachedChar = m_CacheMap[charcode].get();
  if (pCachedChar->form()->GetPageObjectList()->empty())
    pCachedChar->ResetForm();
  return pCachedChar;
}

int CPDF_Type3Font::GetCharWidthF(uint32_t charcode) {
  if (charcode >= FX_ArraySize(m_CharWidthL))
    charcode = 0;

  if (m_CharWidthL[charcode])
    return m_CharWidthL[charcode];

  const CPDF_Type3Char* pChar = LoadChar(charcode);
  return pChar ? pChar->width() : 0;
}

FX_RECT CPDF_Type3Font::GetCharBBox(uint32_t charcode) {
  FX_RECT ret;
  const CPDF_Type3Char* pChar = LoadChar(charcode);
  if (pChar)
    ret = pChar->bbox();
  return ret;
}
