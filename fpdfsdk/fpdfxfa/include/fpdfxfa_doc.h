// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_FPDFXFA_INCLUDE_FPDFXFA_DOC_H_
#define FPDFSDK_FPDFXFA_INCLUDE_FPDFXFA_DOC_H_

#include <memory>

#include "fpdfsdk/fpdfxfa/include/cpdfxfa_docenvironment.h"
#include "xfa/fxfa/include/xfa_ffdoc.h"

class CPDFXFA_App;
class CPDFXFA_Page;
class CPDFSDK_Document;
class CPDFDoc_Environment;
class CXFA_FFDocHandler;

enum LoadStatus {
  FXFA_LOADSTATUS_PRELOAD = 0,
  FXFA_LOADSTATUS_LOADING,
  FXFA_LOADSTATUS_LOADED,
  FXFA_LOADSTATUS_CLOSING,
  FXFA_LOADSTATUS_CLOSED
};

class CPDFXFA_Document {
 public:
  CPDFXFA_Document(std::unique_ptr<CPDF_Document> pPDFDoc,
                   CPDFXFA_App* pProvider);
  ~CPDFXFA_Document();

  FX_BOOL LoadXFADoc();
  CPDF_Document* GetPDFDoc() { return m_pPDFDoc.get(); }
  CXFA_FFDoc* GetXFADoc() { return m_pXFADoc.get(); }
  CXFA_FFDocView* GetXFADocView() { return m_pXFADocView; }

  int GetPageCount();
  CPDFXFA_Page* GetPage(int page_index);
  CPDFXFA_Page* GetPage(CXFA_FFPageView* pPage);

  void DeletePage(int page_index);
  void RemovePage(CPDFXFA_Page* page);
  int GetDocType() { return m_iDocType; }

  CPDFSDK_Document* GetSDKDocument(CPDFDoc_Environment* pFormFillEnv);

  void ClearChangeMark();

 protected:
  friend class CPDFXFA_DocEnvironment;

  CPDFSDK_Document* GetSDKDoc() { return m_pSDKDoc.get(); }
  int GetOriginalPageCount() const { return m_nPageCount; }
  void SetOriginalPageCount(int count) {
    m_nPageCount = count;
    m_XFAPageList.SetSize(count);
  }

  LoadStatus GetLoadStatus() const { return m_nLoadStatus; }

  CFX_ArrayTemplate<CPDFXFA_Page*>* GetXFAPageList() { return &m_XFAPageList; }

 private:
  void CloseXFADoc(CXFA_FFDocHandler* pDoc) {
    if (pDoc) {
      m_pXFADoc->CloseDoc();
      m_pXFADoc.reset();
      m_pXFADocView = nullptr;
    }
  }

  int m_iDocType;

  std::unique_ptr<CPDF_Document> m_pPDFDoc;
  // |m_pSDKDoc| must be destroyed before |m_pPDFDoc| since it needs to access
  // it to kill focused annotations.
  std::unique_ptr<CPDFSDK_Document> m_pSDKDoc;
  std::unique_ptr<CXFA_FFDoc> m_pXFADoc;
  CXFA_FFDocView* m_pXFADocView;  // not owned.
  CPDFXFA_App* const m_pApp;
  CFX_ArrayTemplate<CPDFXFA_Page*> m_XFAPageList;
  LoadStatus m_nLoadStatus;
  int m_nPageCount;

  // Must be destroy before |m_pSDKDoc|.
  CPDFXFA_DocEnvironment m_DocEnv;
};

#endif  // FPDFSDK_FPDFXFA_INCLUDE_FPDFXFA_DOC_H_
