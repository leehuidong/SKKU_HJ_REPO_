#include "KBRun.hh"
#include "LHHelixTrackFindingTask.hh"
#include "FTHit.hh"

#include <iostream>

//#define DEBUG_STEP
//#define FT_INCLUDE

ClassImp(LHHelixTrackFindingTask)

bool LHHelixTrackFindingTask::Init()
{
  auto run = KBRun::GetRun();
  fPar = run -> GetParameterContainer();
  fTpc = (LHTpc *) (run -> GetDetectorSystem() -> GetTpc());
  fPadPlane = (KBPadPlane *) fTpc -> GetPadPlane();

  fHitArray = (TClonesArray *) run -> GetBranch(fBranchNameHit);
  fHitArray_FT = (TClonesArray *) run -> GetBranch(fBranchNameHit_FT);

  kb_debug <<fBranchNameHit<< " " << fBranchNameHit_FT << endl; 


  fTrackArray = new TClonesArray("KBHelixTrack");
  run -> RegisterBranch(fBranchNameTracklet, fTrackArray, fPersistency);

  fTrackHits = new KBHitArray();
  fCandHits = new KBHitArray();
  fGoodHits = new KBHitArray();
  fBadHits  = new KBHitArray();

  run -> RegisterBranch("TrackHit", fTrackHits, false);
  run -> RegisterBranch("CandHit",  fCandHits,  false);
  run -> RegisterBranch("GoodHit",  fGoodHits,  false);
  run -> RegisterBranch("BadHit",   fBadHits,   false);

  fDefaultScale = fPar -> GetParDouble("LHTF_defaultScale");
  fTrackWCutLL = fPar -> GetParDouble("LHTF_trackWCutLL");
  fTrackWCutHL = fPar -> GetParDouble("LHTF_trackWCutHL");
  fTrackHCutLL = fPar -> GetParDouble("LHTF_trackHCutLL");
  fTrackHCutHL = fPar -> GetParDouble("LHTF_trackHCutHL");
  fReferenceAxis = fPar -> GetParAxis("LHTF_refAxis");

  fNextStep = StepNo::kStepInitArray;

  return true;
}

void LHHelixTrackFindingTask::Exec(Option_t*)
{
  fNextStep = StepNo::kStepInitArray;
  while (ExecStep()) {}
}

bool LHHelixTrackFindingTask::ExecStep()
{
       if (fNextStep==kStepEndOfEvent)          return false; 
  else if (fNextStep==kStepInitArray)           fNextStep = StepInitArray();
  else if (fNextStep==kStepNewTrack)            fNextStep = StepNewTrack();
  else if (fNextStep==kStepRemoveTrack)         fNextStep = StepRemoveTrack();
  else if (fNextStep==kStepInitTrack)           fNextStep = StepInitTrack();
  else if (fNextStep==kStepInitTrackAddHit)     fNextStep = StepInitTrackAddHit();
  else if (fNextStep==kStepContinuum)           fNextStep = StepContinuum();
  else if (fNextStep==kStepContinuumAddHit)     fNextStep = StepContinuumAddHit();
  else if (fNextStep==kStepExtrapolation)       fNextStep = StepExtrapolation();
  else if (fNextStep==kStepExtrapolationAddHit) fNextStep = StepExtrapolationAddHit();
  else if (fNextStep==kStepConfirmation)        fNextStep = StepConfirmation();
  else if (fNextStep==kStepFinalizeTrack)       fNextStep = StepFinalizeTrack();
  else if (fNextStep==kStepNextPhase)           fNextStep = StepNextPhase();
  else if (fNextStep==kStepEndEvent)            fNextStep = StepEndEvent();

#ifdef DEBUG_STEP
       if (fNextStep==kStepInitArray)           kb_debug << "NextStep is StepInitArray" << endl;
  else if (fNextStep==kStepNewTrack)            kb_debug << "NextStep is StepNewTrack" << endl;
  else if (fNextStep==kStepRemoveTrack)         kb_debug << "NextStep is StepRemoveTrack" << endl;
  else if (fNextStep==kStepInitTrack)           kb_debug << "NextStep is StepInitTrack" << endl;
  else if (fNextStep==kStepInitTrackAddHit)     kb_debug << "NextStep is StepInitTrackAddHit" << endl;
  else if (fNextStep==kStepContinuum)           kb_debug << "NextStep is StepContinuum" << endl;
  else if (fNextStep==kStepContinuumAddHit)     kb_debug << "NextStep is StepContinuumAddHit" << endl;
  else if (fNextStep==kStepExtrapolation)       kb_debug << "NextStep is StepExtrapolation" << endl;
  else if (fNextStep==kStepExtrapolationAddHit) kb_debug << "NextStep is StepExtrapolationAddHit" << endl;
  else if (fNextStep==kStepConfirmation)        kb_debug << "NextStep is StepConfirmation" << endl;
  else if (fNextStep==kStepFinalizeTrack)       kb_debug << "NextStep is StepFinalizeTrack" << endl;
  else if (fNextStep==kStepNextPhase)           kb_debug << "NextStep is StepNextPhase" << endl;
  else if (fNextStep==kStepEndEvent)            kb_debug << "NextStep is StepEndEvent" << endl;

  if (fCurrentTrack != nullptr)
    fCurrentTrack -> Print();
#endif
  return true;
}

bool LHHelixTrackFindingTask::ExecStepUptoTrackNum(Int_t numTracks)
{
  if (fNextStep==kStepEndOfEvent)
    return false;

  while (ExecStep()) {
    if (fNextStep==kStepNewTrack && fTrackArray -> GetEntriesFast() >= numTracks)
      break;
  }

  return true;
}

int LHHelixTrackFindingTask::StepInitArray()
{
#ifdef DEBUG_STEP
  kb_debug << "StepInitArray" << endl;
#endif
  fCurrentTrack = nullptr;

  fPhaseIndex = 0;

  fPadPlane -> ResetHitMap();
  fPadPlane -> SetHitArray(fHitArray_FT);

  for(int i = 0; i < fHitArray_FT->GetEntries(); i++)
  {
    FTHit * hit_FT = (FTHit *) fHitArray_FT->At(i);
    
    hit_FT->SetPadID(fPadPlane -> FindPadID(hit_FT->GetX(),hit_FT->GetY()));
  //kb_debug << hit_FT->GetX() << "\t" << hit_FT->GetY() << " " << hit_FT->GetZ() << endl ;//"\t" << fPadPlane -> FindPadID(hit_FT->GetX(),hit_FT->GetY()) << endl;
  //kb_debug << fPadPlane -> FindPadID(hit_FT->GetX(),hit_FT->GetY()) << endl;
    //fPadPlane -> AddHit(hit_FT);
  }
 
  fTrackArray -> Clear("C");
  fTrackHits -> Clear();
  fCandHits -> Clear();
  fGoodHits -> Clear();
  fBadHits -> Clear(); 

  ///return kStepEndOfEvent;
  return kStepNewTrack;
}

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

int LHHelixTrackFindingTask::StepNewTrack()
{
#ifdef DEBUG_STEP
    kb_debug << "StepNewTrack" << endl;
#endif

  fTrackHits -> Clear();
  fCandHits -> Clear();
  fGoodHits -> Clear();
  ReturnBadHitsToPadPlane();

  KBTpcHit *hit = fPadPlane -> PullOutNextFreeHit();
  if (hit == nullptr) {
#ifdef DEBUG_STEP
    kb_debug << "no more hits" << endl;
#endif
    return kStepNextPhase;
  }
  
  Int_t idx = fTrackArray -> GetEntries();
  fCurrentTrack = new ((*fTrackArray)[idx]) KBHelixTrack(idx);
  fCurrentTrack -> SetReferenceAxis(fReferenceAxis);
  fCurrentTrack -> AddHit(hit);
  fGoodHits -> AddHit(hit);
  
  /////kb_info << "GoodHit Enetries : " << fGoodHits->GetEntries() << endl;

/*
  int numHits_FT = fHitArray_FT->GetEntries();
  for(int inum = 0; inum < numHits_FT; inum++)
  {
    KBHit* hit_FT = (KBHit*) fHitArray_FT->At(inum);

  fCurrentTrack -> AddHit(hit_FT);
  fGoodHits -> AddHit(hit_FT);

  }
*/

#ifdef DEBUG_STEP
  fCurrentTrack -> Print(">");
#endif

    return kStepInitTrack;
}

int LHHelixTrackFindingTask::StepRemoveTrack()
{
#ifdef DEBUG_STEP
    kb_debug << "StepRemoveTrack" << endl;
#endif
  fGoodHits -> Clear();
  fTrackHits -> Clear();
  fCandHits -> Clear();
  
  //////kb_info << "Remove Good Hit check ... : " << fGoodHits -> GetEntries() << endl;

#ifdef DEBUG_STEP
    kb_debug << "Return bad hits" << endl;
#endif
  ReturnBadHitsToPadPlane();

  auto trackHits = fCurrentTrack -> GetHitArray();
  Int_t numTrackHits = trackHits -> GetNumHits();
#ifdef DEBUG_STEP
    kb_debug << "Return track hits" << endl;
    kb_debug << numTrackHits << endl;
#endif
  for (Int_t iTrackHit = 0; iTrackHit < numTrackHits; ++iTrackHit) {
    auto trackHit = (KBTpcHit *) trackHits -> GetHit(iTrackHit);
#ifdef DEBUG_STEP
    kb_debug << iTrackHit << endl;
    trackHit -> Print();
#endif
    trackHit -> AddTrackCand(-1);
    fPadPlane -> AddHit(trackHit);
  }
  fTrackArray -> Remove(fCurrentTrack);
#ifdef DEBUG_STEP
    kb_debug << "return" << endl;
#endif

  fCurrentTrack = nullptr;

  /////kb_info << "This track is coming from [RemoveTrack]" << endl;
  return kStepNewTrack;
}

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

int LHHelixTrackFindingTask::StepInitTrack()
{
  double vx,vy,vz,vr;
  auto VectorTest = fCurrentTrack -> GetMean();
  vx = VectorTest.X();
  vy = VectorTest.Y();
  vz = VectorTest.Z();
  vr = sqrt(vx*vx + vy*vy + vz*vz);
  
  //cout << "Current Track Mean \t" << " x : " << vx << "\t y : " << vy << "\t z : " << vz << "\t r : " << vr << endl;  
#ifdef DEBUG_STEP
  kb_debug << "StepInitTrack" << endl;
#endif
  fCandHits -> Clear();
  /////kb_info << "# of fCandHits(after clear) : " << fCandHits -> GetEntriesFast() << endl;
  /////kb_info << "# of fGoodHits(before pullout) : " << fGoodHits -> GetEntriesFast() << endl;
  
  fPadPlane -> PullOutNeighborHits(fGoodHits, fCandHits);
  /////kb_info << "# of fCandHits(after pullout) : " << fCandHits -> GetEntriesFast() << endl;
    
  fGoodHits -> MoveHitsTo(fTrackHits);
  /////kb_info << "Trackhit entries : " << fTrackHits->GetEntries() << endl;

#ifdef DEBUG_STEP
  kb_debug << "After pull hits" << endl;
#endif
  fNumCandHits = fCandHits -> GetEntriesFast();
  if (fNumCandHits == 0) return kStepRemoveTrack;
  
  /////fCandHits -> SortByDistanceTo(fCurrentTrack -> GetMean(), true);

  /*double x,y,z,r;
  cout << "=================== before sorting ... =====================" << endl;
  for(int i = 0; i < fNumCandHits; i++)
  {
    auto testCandHit = (KBHit*) fCandHits->At(i);
    x = testCandHit->GetX();
    y = testCandHit->GetY();
    z = testCandHit->GetZ();
    r = sqrt(x*x+y*y+z*z);
    cout << "inum : " << i << "\t" << "x : " << x << "\t" << "y : " << y << "\t" << "z : " << z << "\t" << "r : " << r << "\t distance : " << (r-vr) << endl;
  }
  */
  fCandHits -> SortByDistanceTo(fCurrentTrack -> GetMean(), true);
  
  
  /*cout << "=================== after sorting ... =====================" << endl;
  for(int i = 0; i < fNumCandHits; i++)
  {
    auto testCandHit2 = (KBHit*) fCandHits->At(i);
    x = testCandHit2->GetX();
    y = testCandHit2->GetY();
    z = testCandHit2->GetZ();
    r = sqrt(x*x+y*y+z*z);
    cout << "inum : " << i << "\t" << "x : " << x << "\t" << "y : " << y << "\t" << "z : " << z << "\t" << "r : " << r << "\t distance : " << (r-vr) << endl;
  }
  cout << endl << endl << endl;
  */

#ifdef DEBUG_STEP
  kb_debug << "After sort" << endl;
#endif
 
  return kStepInitTrackAddHit;
}

int LHHelixTrackFindingTask::StepInitTrackAddHit()
{
#ifdef DEBUG_STEP
  kb_debug << "StepInitTrackAddHit, fNumCandHits=" << fNumCandHits << endl;
#endif
  auto candHit = (KBTpcHit *) fCandHits -> GetLastHit();
  //cout << "LastHit XYZ (will be deleted) :: " <<candHit->GetX() << "\t" << candHit->GetY() << "\t" << candHit->GetZ() << endl; ///// 가장 GoodHit과 가까운 Hit
  fCandHits -> RemoveLastHit();

  Double_t quality;
  if (fCurrentTrack -> IsHelix())
  {  quality = CorrelateHitWithTrack(fCurrentTrack, candHit);
  //  cout << "IsHelix quality : " << quality << endl;
  }
  else
  {  
    quality = CorrelateHitWithTrackCandidate(fCurrentTrack, candHit);
  //  cout << "Not IsHelix quality : " << quality << endl;
  }
  if (quality > 0) {
    fGoodHits -> AddHit(candHit);
    fCurrentTrack -> AddHit(candHit);
    fCurrentTrack -> FitPlane(); // XXX should comment out

    auto numHitsInTrack = fCurrentTrack -> GetNumHits();
#ifdef DEBUG_STEP
      kb_debug << "Quality is " << quality << ", hit is added, numHitsInTrack=" << numHitsInTrack << endl;
#endif

    if (numHitsInTrack > fCutMaxNumHitsInitTrack) {
#ifdef DEBUG_STEP
      kb_debug << "track has too many hits. remove track" << endl;
#endif
      Int_t numCandHits2 = fCandHits -> GetEntriesFast();
      for (Int_t iCand = 0; iCand < numCandHits2; ++iCand)
        fPadPlane -> AddHit((KBTpcHit *) fCandHits -> GetHit(iCand));
      fCandHits -> Clear("C");
      return kStepRemoveTrack;
    }

    if (numHitsInTrack >= fMinHitsToFitInitTrack) {
#ifdef DEBUG_STEP
      kb_debug << "fit track since track has enough number of hits" << endl;
#endif
      //fCurrentTrack -> SetIsHelix();
      fCurrentTrack -> Fit();
#ifdef DEBUG_STEP
      kb_debug << "After fit, " << endl;
      fCurrentTrack -> Print();
      kb_debug << "After fit, track radius : " << fCurrentTrack -> GetHelixRadius() << " >? " << fCutMinHelixRadius << endl;
      kb_debug << "RMS  " << fCurrentTrack -> GetRMS () << endl;
      kb_debug << "RMSR " << fCurrentTrack -> GetRMSR() << endl;
      kb_debug << "RMST " << fCurrentTrack -> GetRMST() << endl;
      kb_debug << "After fit, track length : " << fCurrentTrack -> TrackLength() << " >? " << fTrackLengthCutScale << " * " << fCurrentTrack -> GetRMST() << endl;
      kb_debug << "... " << fTrackLengthCutScale * fCurrentTrack -> GetRMST() << endl;
#endif
      if (numHitsInTrack > fCutMinNumHitsInitTrack &&
          fCurrentTrack -> GetHelixRadius() > fCutMinHelixRadius &&
          fCurrentTrack -> TrackLength() > fTrackLengthCutScale * fCurrentTrack -> GetRMST()) {
#ifdef DEBUG_STEP
        kb_debug << "Track is good" << endl;
#endif
        return kStepContinuum;
      }
      else {
#ifdef DEBUG_STEP
        kb_debug << "Track need more hits" << endl;
#endif
        fCurrentTrack -> FitPlane();
      }
    }

#ifdef DEBUG_STEP
    kb_debug << "Quality is " << quality << ", track fit, numHitsInTrack=" << numHitsInTrack  << endl;
#endif
    //fCurrentTrack -> FitPlane(); // XXX should comment in
  }
  else {
#ifdef DEBUG_STEP
    kb_debug << "Quality is " << quality << ", bad hit" << endl;
#endif
    fBadHits -> AddHit(candHit);
  }

  fNumCandHits = fCandHits -> GetEntriesFast();
  if (fNumCandHits==0)
  { /////kb_info << "This track is coming frome [InitTrackAddHit]" << endl;
    //cout << "after init track" << endl;
    return kStepInitTrack;
  }
  //cout << "after init track addhit" << endl;
  return kStepInitTrackAddHit;
}

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

int LHHelixTrackFindingTask::StepContinuum()
{ 
#ifdef DEBUG_STEP
  kb_debug << "StepContinuum" << endl;
  kb_debug << "goodhits" << endl;
  fGoodHits -> PrintHits(1);
#endif
  fPadPlane -> PullOutNeighborHits(fGoodHits, fCandHits);
  fGoodHits -> MoveHitsTo(fTrackHits);

  fNumCandHits = fCandHits -> GetEntries();
  if (fNumCandHits==0)
    return kStepExtrapolation;

  fCandHits -> SortByCharge(false);
  return kStepContinuumAddHit;
}

int LHHelixTrackFindingTask::StepContinuumAddHit()
{
#ifdef DEBUG_STEP
  kb_debug << "StepContinuumAddHit" << endl;
#endif
  for (Int_t iHit = 0; iHit < fNumCandHits; iHit++) {
    KBTpcHit *candHit = (KBTpcHit *) fCandHits -> GetLastHit();
    fCandHits -> RemoveLastHit();

    Double_t quality = 0; 
    if (CheckParentTrackID(candHit) == -2)
      quality = CorrelateHitWithTrack(fCurrentTrack, candHit);

    if (quality > 0) {
      fGoodHits -> AddHit(candHit);
      fCurrentTrack -> AddHit(candHit);
      fCurrentTrack -> Fit();
#ifdef DEBUG_STEP
      kb_debug << iHit << "(/" << fNumCandHits << ") is good, quality is " << quality << endl;
#endif
    } else {
      fBadHits -> AddHit(candHit);
#ifdef DEBUG_STEP
      kb_debug << iHit << "(/" << fNumCandHits << ") is bad" << endl;
#endif
    }
  }

  return kStepContinuum;
}

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

int LHHelixTrackFindingTask::StepExtrapolation()
{ 
#ifdef DEBUG_STEP
  kb_debug << "StepExtrapolation" << endl;
#endif
  ReturnBadHitsToPadPlane();

  return kStepExtrapolationAddHit;
}

int LHHelixTrackFindingTask::StepExtrapolationAddHit()
{
#ifdef DEBUG_STEP
  kb_debug << "StepExtrapolationAddHit" << endl;
#endif
  Int_t countIteration = 0;
  bool buildFromHeadOrTail = true;
  Double_t extrapolationLength = 0;
  while (AutoBuildByExtrapolation(fCurrentTrack, buildFromHeadOrTail, extrapolationLength)) {
    if (++countIteration > 200)
      break;
  }

  countIteration = 0;
  buildFromHeadOrTail = !buildFromHeadOrTail;
  extrapolationLength = 0;
  while (AutoBuildByExtrapolation(fCurrentTrack, buildFromHeadOrTail, extrapolationLength)) {
    if (++countIteration > 200)
      break;
  }

  ReturnBadHitsToPadPlane();

  bool isGood = CheckTrackQuality(fCurrentTrack);
  //cout << "IS GOOD?? " << isGood << endl;
  if (isGood) {
    return kStepConfirmation;
#ifdef DEBUG_STEP
    kb_debug << "Track is good! " << endl;
#endif
  }

#ifdef DEBUG_STEP
    kb_debug << "Track is bad! " << endl;
#endif

  return kStepRemoveTrack;
}

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

int LHHelixTrackFindingTask::StepConfirmation()
{
#ifdef DEBUG_STEP
  kb_debug << "StepConfirmation" << endl;
#endif

  bool tailToHead = false;
  KBVector3 pTail(fCurrentTrack -> PositionAtTail(), fReferenceAxis);
  KBVector3 pHead(fCurrentTrack -> PositionAtHead(), fReferenceAxis);

  if (pHead.K() > pHead.K())
    tailToHead = true;

  ReturnBadHitsToPadPlane();
#ifdef DEBUG_STEP
  kb_debug << "First Try" << endl;
  fCurrentTrack -> Print(">");
#endif
  if (BuildAndConfirmTrack(fCurrentTrack, tailToHead) == false) {
#ifdef DEBUG_STEP
    kb_debug << "failed in confirm!" << endl;
    fCurrentTrack -> Print(">");
#endif
    return kStepRemoveTrack;
  }

  tailToHead = !tailToHead; 

  ReturnBadHitsToPadPlane();
#ifdef DEBUG_STEP
  kb_debug << "Second Try" << endl;
#endif
  if (BuildAndConfirmTrack(fCurrentTrack, tailToHead) == false)
    return kStepRemoveTrack;

#ifdef DEBUG_STEP
  kb_debug << "End" << endl;
#endif

  ReturnBadHitsToPadPlane();

  return kStepFinalizeTrack;
}


int LHHelixTrackFindingTask::StepFinalizeTrack()
{
  auto trackHits = fCurrentTrack -> GetHitArray();
  Int_t trackID = fCurrentTrack -> GetTrackID();
  Int_t numTrackHits = trackHits -> GetNumHits();
  for (Int_t iTrackHit = 0; iTrackHit < numTrackHits; ++iTrackHit) {
    auto trackHit = (KBTpcHit *) trackHits -> GetHit(iTrackHit);
    trackHit -> AddTrackCand(trackID);
    fPadPlane -> AddHit(trackHit);
  }

  fGoodHits -> MoveHitsTo(fTrackHits);
  fGoodHits -> Clear();

  /////kb_info << "This track is coming from [FinalizeTrack]" << endl;
  return kStepNewTrack;
}

int LHHelixTrackFindingTask::StepNextPhase()
{
  if (fPhaseIndex==0) {
    fPhaseIndex = 1;

    fPadPlane -> ResetEvent();

    fTrackHits -> Clear();
    fCandHits -> Clear();
    fGoodHits -> Clear();
    ReturnBadHitsToPadPlane();

    //fMinHitsToFitInitTrack = 7;
    //fCutMinNumHitsInitTrack = 10;
    fCutMaxNumHitsInitTrack = 25;
    //fCutMinNumHitsFinalTrack = 15;
    //fCutMinHelixRadius = 30.;
    //fTrackLengthCutScale = 2.5;
    //fCutdkInExpectedTrackPath = 4.;

  /////kb_info << "This track is coming from [NextPhaseTrack]" << endl;
    return kStepNewTrack;
  }
  else if (fPhaseIndex==1)
    return kStepEndEvent;
}

int LHHelixTrackFindingTask::StepEndEvent()
{
  fTrackArray -> Compress();

  Int_t numTracks = fTrackArray -> GetEntriesFast();
  for (Int_t iTrack = 0; iTrack < numTracks; ++iTrack) {
    KBHelixTrack *track = (KBHelixTrack *) fTrackArray -> At(iTrack);
    track -> SetTrackID(iTrack);
    track -> FinalizeHits();
  }

  kb_info << "Number of found tracks: " << fTrackArray -> GetEntries() << endl;

  return kStepEndOfEvent;
}

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

void LHHelixTrackFindingTask::ReturnBadHitsToPadPlane()
{
  fNumBadHits = fBadHits -> GetEntriesFast();
  for (Int_t iBad = 0; iBad < fNumBadHits; ++iBad)
    fPadPlane -> AddHit((KBTpcHit *) fBadHits -> GetHit(iBad));
  fBadHits -> Clear();
}

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

double LHHelixTrackFindingTask::CorrelateHitWithTrackCandidate(KBHelixTrack *track, KBTpcHit *hit)
{
#ifdef DEBUG_STEP
  kb_debug << "======================================== track hits" << endl;
  fCurrentTrack -> Print(">");
  kb_debug << "======================================== hit" << endl;
  hit -> PrintTpcHit();
  kb_debug << "========================================" << endl;
#endif

  if (hit -> GetNumTrackCands() != 0)
    return 0;

  Double_t quality = 0;

  auto row = hit -> GetRow();
  auto layer = hit -> GetLayer();

  KBVector3 qosHit(hit -> GetPosition(), fReferenceAxis);
  auto trackHits = track -> GetHitArray();
  bool passCutdk = false;
  Int_t numTrackHits = trackHits -> GetNumHits();
#ifdef DEBUG_STEP
    kb_debug << numTrackHits << endl;
#endif
  for (Int_t iTrackHit = 0; iTrackHit < numTrackHits; ++iTrackHit) {
    auto trackHit = (KBTpcHit *) trackHits -> GetHit(iTrackHit);
    if (row == trackHit -> GetRow() && layer == trackHit -> GetLayer())
      continue; // XXX
    KBVector3 qosTrackHit(trackHit -> GetPosition(), fReferenceAxis);
    auto di = qosHit.I() - qosTrackHit.I();
    auto dj = qosHit.J() - qosTrackHit.J();
    auto distPadCenter = sqrt(di*di + dj*dj);
    Double_t tangentDip = abs(qosTrackHit.K())/sqrt(qosTrackHit.I()*qosTrackHit.I()+qosTrackHit.J()*qosTrackHit.J());
    Double_t dkInExpectedTrackPath = 1.2 * distPadCenter * tangentDip;
    Double_t dkBetweenTwoHits = abs(qosHit.K()-qosTrackHit.K());

    if (dkInExpectedTrackPath < fCutdkInExpectedTrackPath) dkInExpectedTrackPath = fCutdkInExpectedTrackPath;
#ifdef DEBUG_STEP
    kb_debug << "TrackHit-" << iTrackHit << ": dkBetweenTwoHits and dkInExpectedTrackPath is " << dkBetweenTwoHits << " <? " << dkInExpectedTrackPath << endl;
#endif
    if (dkBetweenTwoHits < dkInExpectedTrackPath) {
      passCutdk = true;
    }
  }

  if (!passCutdk) {
    return 0;
  }

  if (track -> IsBad()) {
#ifdef DEBUG_STEP
    kb_debug << "track was bad" << endl;
#endif
    quality = 1;
  }
  else if (track -> IsLine()) {
#ifdef DEBUG_STEP
    kb_debug << "track was line" << endl;
#endif
    KBVector3 perp = track -> PerpLine(hit -> GetPosition());

    Double_t rmsCut = track -> GetRMST();
    if (rmsCut < fTrackHCutLL) rmsCut = fTrackHCutLL;
    if (rmsCut > fTrackHCutHL) rmsCut = fTrackHCutHL;
    rmsCut = 3 * rmsCut;

    if (perp.K() > rmsCut) {
      quality = 0;
    }
    else {
      perp.SetK(0);
      auto magcut = 15.;
      if (perp.Mag() < magcut)
      //if (perp.Mag() < 10*pos1.K()/sqrt(pos1.Mag()))
      {
        quality = 1;
      }
    }
  }
  else if (track -> IsPlane()) {
#ifdef DEBUG_STEP
    kb_debug << "track was plane" << endl;
#endif
    Double_t dist = (track -> PerpPlane(hit -> GetPosition())).Mag();

    Double_t rmsCut = track -> GetRMST();
    if (rmsCut < fTrackHCutLL) rmsCut = fTrackHCutLL;
    if (rmsCut > fTrackHCutHL) rmsCut = fTrackHCutHL;
    rmsCut = 3 * rmsCut;

#ifdef DEBUG_STEP
    kb_debug << "dist is " <<  dist << endl;
    kb_debug << "track rmsCut is " <<  rmsCut << "(" << track -> GetRMST() << ")" << endl;
#endif
    if (dist < rmsCut) {
      quality = 1;
    }
  }
  else {
#ifdef DEBUG_STEP
    kb_debug << "track is ..." << endl;
    track -> Print(">");
#endif
  }

  return quality;
}

double LHHelixTrackFindingTask::CorrelateHitWithTrack(KBHelixTrack *track, KBTpcHit *hit, Double_t rScale)
{
  Double_t scale = rScale * fDefaultScale;
  Double_t trackLength = track -> TrackLength();
  if (trackLength < 500.)
    scale = scale + (500. - trackLength)/500.;

  /*
  auto direction = track->Momentum().Unit();
  Double_t dot = abs(direction.Dot(KBVector3(fReferenceAxis,0,0,1).GetXYZ()));
  auto a = 1.;
  auto b = 1.;
  if (dot > .5) {
     a = (2*(dot-.5)+1); //
     b = (2*(dot-.5)+1); //
  }
  auto trackHCutLL = a*fTrackHCutLL;
  auto trackHCutHL = a*fTrackHCutHL;
  auto trackWCutLL = b*fTrackWCutLL;
  auto trackWCutHL = b*fTrackWCutHL;
  */
  auto trackHCutLL = fTrackHCutLL;
  auto trackHCutHL = fTrackHCutHL;
  auto trackWCutLL = fTrackWCutLL;
  auto trackWCutHL = fTrackWCutHL;

  Double_t rmsWCut = track -> GetRMSR();
  if (rmsWCut < trackWCutLL) rmsWCut = trackWCutLL;
  if (rmsWCut > trackWCutHL) rmsWCut = trackWCutHL;
  rmsWCut = scale * rmsWCut;

  Double_t rmsHCut = track -> GetRMST();
  if (rmsHCut < trackHCutLL) rmsHCut = trackHCutLL;
  if (rmsHCut > trackHCutHL) rmsHCut = trackHCutHL;
  rmsHCut = scale * rmsHCut;

  TVector3 qHead = track -> Map(track -> PositionAtHead());
  TVector3 qTail = track -> Map(track -> PositionAtTail());
  TVector3 q = track -> Map(hit -> GetPosition());

  if (qHead.Z() > qTail.Z()) {
    if (CheckHitDistInAlphaIsLargerThanQuarterPi(track, q.Z() - qHead.Z())) return 0;
    if (CheckHitDistInAlphaIsLargerThanQuarterPi(track, qTail.Z() - q.Z())) return 0;
  } else {
    if (CheckHitDistInAlphaIsLargerThanQuarterPi(track, q.Z() - qTail.Z())) return 0;
    if (CheckHitDistInAlphaIsLargerThanQuarterPi(track, qHead.Z() - q.Z())) return 0;
  }

  Double_t dr = abs(q.X());
  Double_t quality = 0;
  if (dr < rmsWCut && abs(q.Y()) < rmsHCut)
    quality = sqrt((dr-rmsWCut)*(dr-rmsWCut)) / rmsWCut;

  return quality;
}

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

bool LHHelixTrackFindingTask::BuildAndConfirmTrack(KBHelixTrack *track, bool &tailToHead)
{
  track -> SortHits(!tailToHead);
  auto trackHits = track -> GetHitArray();
  Int_t numHits = trackHits -> GetNumHits();

  TVector3 q, m;
  //Double_t lPre = track -> ExtrapolateByMap(trackHits->at(numHits-1)->GetPosition(), q, m);

  Double_t extrapolationLength = 10.;
  for (Int_t iHit = 1; iHit < numHits; iHit++) 
  {
    //auto trackHit = (KBTpcHit *) trackHits -> GetHit(numHits-iHit-1);
    auto trackHit = (KBTpcHit *) trackHits -> GetLastHit();
    //Double_t lCur = track -> ExtrapolateByMap(trackHit->GetPosition(), q, m);

    Double_t quality = CorrelateHitWithTrack(track, trackHit);

    if (quality <= 0) {
      track -> RemoveHit(trackHit);
      trackHit -> RemoveTrackCand(trackHit -> GetTrackID());
#ifdef DEBUG_STEP
      kb_debug << "after remove hit" << endl;
      track -> Print();
#endif
      Int_t helicity = track -> Helicity();
      track -> Fit();
      if (helicity != track -> Helicity())
        tailToHead = !tailToHead;

      continue;
    }

    /*
    Double_t dLength = abs(lCur - lPre);
    extrapolationLength = 10;
    while(dLength > 0 && AutoBuildByInterpolation(track, tailToHead, extrapolationLength, 1)) { dLength -= 10; }
    */
  }

  extrapolationLength = 0;
  while (AutoBuildByExtrapolation(track, tailToHead, extrapolationLength)) {}

  if (track -> GetNumHits() < fCutMinNumHitsFinalTrack)
    return false;

  return true;
}

bool LHHelixTrackFindingTask::AutoBuildByExtrapolation(KBHelixTrack *track, bool &buildHead, Double_t &extrapolationLength)
{
  if (track -> GetNumHits() < fCutMinNumHitsFinalTrack)
    return false;

  TVector3 p;
#ifdef DEBUG_STEP
  track -> Print();
#endif
  if (buildHead) p = track -> ExtrapolateHead(extrapolationLength);
  else           p = track -> ExtrapolateTail(extrapolationLength);

  return AutoBuildAtPosition(track, p, buildHead, extrapolationLength);
}

bool LHHelixTrackFindingTask::AutoBuildAtPosition(KBHelixTrack *track, TVector3 p, bool &tailToHead, Double_t &extrapolationLength, Double_t rScale)
{
  KBVector3 p2(p,fReferenceAxis);
  if (fPadPlane -> IsInBoundary(p2.I(), p2.J()) == false)
    return false;

  Int_t helicity = track -> Helicity();

  Double_t rms = 3*track -> GetRMSR();
  if (rms < 25) 
    rms = 25;

  Int_t range = Int_t(rms/8);
  fPadPlane -> PullOutNeighborHits(p2.I(), p2.J(), range, fCandHits);

  fNumCandHits = fCandHits -> GetEntriesFast();
  Bool_t foundHit = false;

  if (fNumCandHits != 0) 
  {
    fCandHits -> SortByCharge(false);

    for (Int_t iHit = 0; iHit < fNumCandHits; iHit++) {
      KBTpcHit *candHit = (KBTpcHit *) fCandHits -> GetLastHit();
      fCandHits -> RemoveLastHit();
      TVector3 pos = candHit -> GetPosition();

      Double_t quality = 0; 
      if (CheckParentTrackID(candHit) < 0) 
        quality = CorrelateHitWithTrack(track, candHit, rScale);

      if (quality > 0) {
        track -> AddHit(candHit);
        track -> Fit();
        foundHit = true;
      } else
        fBadHits -> AddHit(candHit);
    }
  }

  if (foundHit) {
    extrapolationLength = 10; 
    if (helicity != track -> Helicity())
      tailToHead = !tailToHead;
  }
  else {
    extrapolationLength += 10; 
    if (extrapolationLength > 3 * track -> TrackLength()) {
      return false;
    }
  }

  return true;
}

int LHHelixTrackFindingTask::CheckParentTrackID(KBTpcHit *hit)
{
  vector<Int_t> *candTracks = hit -> GetTrackCandArray();
  Int_t numCandTracks = candTracks -> size();
  if (numCandTracks == 0)
    return -2;

  Int_t trackID = -1;
  for (Int_t iCand = 0; iCand < numCandTracks; ++iCand) {
    Int_t candTrackID = candTracks -> at(iCand);
    if (candTrackID != -1) {
      trackID = candTrackID;
    }
  }

  return trackID;
}

bool LHHelixTrackFindingTask::CheckTrackQuality(KBHelixTrack *track)
{
  /*
  Double_t continuity = CheckTrackContinuity(track);
  if (continuity < .6) {
    if (track -> TrackLength() * continuity < 500)
      return false;
  }
  */

  if (track -> GetHelixRadius() < fCutMinHelixRadius)
    return false;

  return true;
}

double LHHelixTrackFindingTask::CheckTrackContinuity(KBHelixTrack *track)
{
  Int_t numHits = track -> GetNumHits();
  if (numHits < 2)
    return -1;

  track -> SortHits();

  Double_t total = 0;
  Double_t continuous = 0;

  TVector3 qPre, mPre, qCur, mCur; // I need q(position on helix)

  KBVector3 kqPre;
  KBVector3 kqCur;

  auto trackHits = track -> GetHitArray();
  auto pPre = trackHits->GetHit(0)->GetPosition();
  track -> ExtrapolateByMap(pPre, qPre, mPre);
  kqPre = KBVector3(qPre,fReferenceAxis);

  auto axis1 = fPadPlane -> GetAxis1();
  auto axis2 = fPadPlane -> GetAxis2();

  for (auto iHit = 1; iHit < numHits; iHit++)
  {
    auto pCur = trackHits->GetHit(iHit)->GetPosition();
    track -> ExtrapolateByMap(pCur, qCur, mCur);

    kqCur = KBVector3(qCur,fReferenceAxis);

    auto val1 = kqCur.At(axis1) - kqPre.At(axis1);
    auto val2 = kqCur.At(axis2) - kqPre.At(axis2);

    auto length = sqrt(val1*val1 + val2*val2);

    total += length;
    if (length <= 1.2 * fPadPlane -> PadDisplacement())
      continuous += length;


    kqPre = kqCur;
  }

  return continuous/total;
}

bool LHHelixTrackFindingTask::CheckHitDistInAlphaIsLargerThanQuarterPi(KBHelixTrack *track, Double_t dLength)
{
  if (dLength > 0) {
    if (dLength > .5*track -> TrackLength()) {
      if (abs(track -> AlphaAtTravelLength(dLength)) > .5*TMath::Pi()) {
        return true;
      }
    }
  }
  return false;
}
