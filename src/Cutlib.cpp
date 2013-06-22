/// @file
/// @brief 境界情報計算関数 実装
///

#include "Cutlib.h"
#include "CutBoundary.h"
#include "CutSearch.h"

#ifdef CUTLIB_OCTREE
#include "CutOctree.h"
#endif

#ifdef CUTLIB_TIMING
#include "CutTiming.h"
#endif

#ifdef _OPENMP
#include "omp.h"
#endif

namespace cutlib {

namespace {

/// 配列サイズ, 計算対象領域のチェック.
CutlibReturn checkSize(const char* func_name,
                       const int ista[], const size_t nlen[],
                       const CutPosArray* cutPos, const CutBidArray* cutBid)
{
  if (ista[0] < cutPos->getStartX() ||
      ista[1] < cutPos->getStartY() ||
      ista[2] < cutPos->getStartZ()) {
    std::cerr << "*** " << func_name << ": "
              << "out of the range: ista[]" << std::endl;
    return CL_SIZE_EXCEED;
  }

  if (ista[0] + nlen[0] > cutPos->getStartX() + cutPos->getSizeX() ||
      ista[1] + nlen[1] > cutPos->getStartY() + cutPos->getSizeY() ||
      ista[2] + nlen[2] > cutPos->getStartZ() + cutPos->getSizeZ()) {
    std::cerr << "*** " << func_name << ": "
              << "out of the range: ista[]+nlen[]" << std::endl;
    return CL_SIZE_EXCEED;
  }
  return CL_SUCCESS;
}


/// Polylibのチェック.
CutlibReturn checkPolylib(const char* func_name, const Polylib* pl)
{
  if (pl == 0) {
    std::cerr << "*** " << func_name << ": Polylib not initialized." << std::endl;
    return CL_BAD_POLYLIB;
  }
  return CL_SUCCESS;
}


#ifdef CUTLIB_OCTREE

/// SklTreeのチェック.
CutlibReturn checkTree(const char* func_name, SklTree* tree)
{
  if (tree == 0) {
    std::cerr << "*** " << func_name << ": SklTree not initialized." << std::endl;
    return CL_BAD_SKLTREE;
  }
  return CL_SUCCESS;
}

#endif //CUTLIB_OCTREE

} // namespace ANONYMOUS


/// 交点情報計算: 計算領域指定.
///
///  @param[in] ista 計算基準点開始位置3次元インデクス
///  @param[in] nlen 計算基準点3次元サイズ
///  @param[in] grid GridAccessorクラスオブジェクト
///  @param[in] pl Polylibクラスオブジェクト
///  @param[in,out] cutPos 交点座標配列ラッパ
///  @param[in,out] cutBid 境界ID配列ラッパ
///
CutlibReturn CalcCutInfo(const int ista[], const size_t nlen[],
                         const GridAccessor* grid, const Polylib* pl,
                         CutPosArray* cutPos, CutBidArray* cutBid)
{
  { 
    // check input parameters
    CutlibReturn ret;
    ret = checkSize("CalcCutInfo", ista, nlen, cutPos, cutBid);
    if (ret != CL_SUCCESS) return ret;
    ret = checkPolylib("CalcCutInfo", pl);
    if (ret != CL_SUCCESS) return ret;
  }

#ifdef CUTLIB_TIMING
  Timer::Start(TOTAL);
#endif

  CutBoundaries* bList = CutBoundary::createCutBoundaryList(pl);

  CutSearch* cutSearch = new CutSearch(pl, bList);

  cutPos->clear();
  cutBid->clear();

#ifdef CUTLIB_TIMING
  Timer::Start(MAIN_LOOP);
#endif
#pragma omp parallel for schedule(dynamic), collapse(3)
  for (int k = ista[2]; k < ista[2]+nlen[2]; k++) {
    for (int j = ista[1]; j < ista[1]+nlen[1]; j++) {
      for (int i = ista[0]; i < ista[0]+nlen[0]; i++) {
        double pos6[6];
        float pos6_f[6];
        BidType bid6[6];
        Triangle* tri6[6];
        double center[3];
        double range[6];

#ifdef CUTLIB_TIMING
        Timer::Start(THREAD_TOTAL);
#endif
        grid->getSearchRange(i, j, k, center, range);
        cutSearch->search(center, range, pos6, bid6, tri6);

        for (int d = 0; d < 6; d++) pos6_f[d] = (float)(pos6[d]/range[d]);

        cutPos->setPos(i, j, k, pos6_f);
        cutBid->setBid(i, j, k, bid6);

#ifdef CUTLIB_TIMING
        Timer::Stop(THREAD_TOTAL);
#endif
      }
    }
  }
#ifdef CUTLIB_TIMING
  Timer::Stop(MAIN_LOOP);
#endif

  delete cutSearch;
  delete bList;

#ifdef CUTLIB_TIMING
  Timer::Stop(TOTAL);
  Timer::Print(TOTAL, "Total");
  Timer::Print(MAIN_LOOP, "Main Loop");
  Timer::PrintFull(THREAD_TOTAL, "Theread Total");
  Timer::PrintFull(SEARCH_POLYGON, "Polylib::search_polygons");
#endif

  return CL_SUCCESS;
}


#ifdef CUTLIB_OCTREE

/// 交点情報計算: Octree, リーフセルのみ.
///
///  @param[in,out] tree SklTreeクラスオブジェクト
///  @param[in] pl Polylibクラスオブジェクト
///  @param cutPos 交点座標データアクセッサ
///  @param cutBid 境界IDデータアクセッサ
///
CutlibReturn CalcCutInfoOctreeLeafCell(SklTree* tree, const Polylib* pl,
                              CutPosOctree* cutPos, CutBidOctree* cutBid)
{
  { 
    // check input parameters
    CutlibReturn ret;
    ret = checkPolylib("CalcCutInfoOctreeLeafCell", pl);
    if (ret != CL_SUCCESS) return ret;
    ret = checkTree("CalcCutInfoOctreeLeafCell", tree);
    if (ret != CL_SUCCESS) return ret;
  }

#ifdef CUTLIB_TIMING
  Timer::Start(TOTAL);
#endif

  CutBoundaries* bList = CutBoundary::createCutBoundaryList(pl);

  CutSearch* cutSearch = new CutSearch(pl, bList);

#ifdef CUTLIB_TIMING
  Timer::Start(MAIN_LOOP);
#endif
  for (SklCell* cell = tree->GetLeafCellFirst(); cell != 0;
       cell = tree->GetLeafCellNext(cell)) {
    double pos6[6];
    float pos6_f[6];
    BidType bid6[6];
    Triangle* tri6[6];
    double center[3];
    double range[6];

    cutPos->assignData(cell->GetData());
    cutBid->assignData(cell->GetData());

    cutOctree::getSearchRange(cell, center, range);
    cutSearch->search(center, range, pos6, bid6, tri6);

    for (int d = 0; d < 6; d++) pos6_f[d] = (float)(pos6[d]/range[d]);

    cutPos->setPos(pos6_f);
    cutBid->setBid(bid6);
  }
#ifdef CUTLIB_TIMING
  Timer::Stop(MAIN_LOOP);
#endif

  delete cutSearch;
  delete bList;

#ifdef CUTLIB_TIMING
  Timer::Stop(TOTAL);
  Timer::Print(TOTAL, "Total");
  Timer::Print(MAIN_LOOP, "Main Loop");
  Timer::Print(SEARCH_POLYGON, "Polylib::search_polygons");
#endif

  return CL_SUCCESS;
}


/// 交点情報計算: Octree, 全セル計算.
///
///  @param[in,out] tree SklTreeクラスオブジェクト
///  @param[in] pl Polylibクラスオブジェクト
///  @param cutPos 交点座標データアクセッサ
///  @param cutBid 境界IDデータアクセッサ
///
CutlibReturn CalcCutInfoOctreeAllCell(SklTree* tree, const Polylib* pl,
                              CutPosOctree* cutPos, CutBidOctree* cutBid)
{
  { 
    // check input parameters
    CutlibReturn ret;
    ret = checkPolylib("CalcCutInfoOctreeAllCell", pl);
    if (ret != CL_SUCCESS) return ret;
    ret = checkTree("CalcCutInfoOctreeAllCell", tree);
    if (ret != CL_SUCCESS) return ret;
  }

#ifdef CUTLIB_TIMING
  Timer::Start(TOTAL);
#endif

  CutBoundaries* bList = CutBoundary::createCutBoundaryList(pl);

  size_t nx, ny, nz;
  tree->GetSize(nx, ny, nz);

#ifdef CUTLIB_TIMING
  Timer::Start(MAIN_LOOP);
#endif
  for (size_t k = 0; k < nz; k++) {
    for (size_t j = 0; j < ny; j++) {
      for (size_t i = 0; i < nx; i++) {
        SklCell* rootCell = tree->GetRootCell(i, j, k);
        Vec3f org, d;
        rootCell->GetOrigin(org[0], org[1], org[2]);
        rootCell->GetPitch(d[0], d[1], d[2]);
        Vec3f min = org - 0.5 * d;
        Vec3f max = org + 1.5 * d;

        cutOctree::CutTriangles ctList;
        cutOctree::CutTriangle::AppendCutTriangles(ctList, pl, bList, min, max);

        cutOctree::calcCutInfo(rootCell, org, d, cutPos, cutBid, ctList);

        cutOctree::CutTriangle::DeleteCutTriangles(ctList);
      }
    }
  }
#ifdef CUTLIB_TIMING
  Timer::Stop(MAIN_LOOP);
#endif

  delete bList;

#ifdef CUTLIB_TIMING
  Timer::Stop(TOTAL);
  Timer::Print(TOTAL, "Total");
  Timer::Print(MAIN_LOOP, "Main Loop");
  Timer::Print(SEARCH_POLYGON, "Polylib::search_polygons");
#endif

  return CL_SUCCESS;
}


/// 交点情報計算: Octree, 全セル計算, デバッグ用.
///
/// 全セルでPolylibの検索メソッドを使用
///
///  @param[in,out] tree SklTreeクラスオブジェクト
///  @param[in] pl Polylibクラスオブジェクト
///  @param cutPos 交点座標データアクセッサ
///  @param cutBid 境界IDデータアクセッサ
///
CutlibReturn CalcCutInfoOctreeAllCell0(SklTree* tree, const Polylib* pl,
                              CutPosOctree* cutPos, CutBidOctree* cutBid)
{
  {
    // check input parameters
    CutlibReturn ret;
    ret = checkPolylib("CalcCutInfoOctreeAllCell0", pl);
    if (ret != CL_SUCCESS) return ret;
    ret = checkTree("CalcCutInfoOctreeAllCell0", tree);
    if (ret != CL_SUCCESS) return ret;
  }

#ifdef CUTLIB_TIMING
  Timer::Start(TOTAL);
#endif

  CutBoundaries* bList = CutBoundary::createCutBoundaryList(pl);

  CutSearch* cutSearch = new CutSearch(pl, bList);

  size_t nx, ny, nz;
  tree->GetSize(nx, ny, nz);

#ifdef CUTLIB_TIMING
  Timer::Start(MAIN_LOOP);
#endif
  for (size_t k = 0; k < nz; k++) {
    for (size_t j = 0; j < ny; j++) {
      for (size_t i = 0; i < nx; i++) {
        SklCell* cell = tree->GetRootCell(i, j, k);
        cutOctree::calcCutInfo0(cell, cutSearch, cutPos, cutBid);
      }
    }
  }
#ifdef CUTLIB_TIMING
  Timer::Stop(MAIN_LOOP);
#endif

  delete cutSearch;
  delete bList;

#ifdef CUTLIB_TIMING
  Timer::Stop(TOTAL);
  Timer::Print(TOTAL, "Total");
  Timer::Print(MAIN_LOOP, "Main Loop");
  Timer::Print(SEARCH_POLYGON, "Polylib::search_polygons");
#endif

  return CL_SUCCESS;
}

#endif // CUTLIB_OCTREE

} // namespace cutlib