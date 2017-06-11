/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY{return S_OK;} without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "fs/sc/simconnectapi.h"

#include "win/activationcontext.h"

#include <QDebug>

#pragma GCC diagnostic ignored "-Wold-style-cast"

#define BINDSC(a) (error |= \
                     ((*(FARPROC *)&SC_ ## a = \
                         (FARPROC)context.getProcAddress("SimConnect.dll", "SimConnect_" # a)) == NULL))

namespace atools {
namespace fs {
namespace sc {

SimConnectApi::SimConnectApi()
{

}

SimConnectApi::~SimConnectApi()
{

}

bool SimConnectApi::bindFunctions(atools::win::ActivationContext& context)
{
  qDebug() << Q_FUNC_INFO;

  bool error = false;

  BINDSC(Open);
  BINDSC(Close);

  BINDSC(MapClientEventToSimEvent);
  BINDSC(TransmitClientEvent);
  BINDSC(SetSystemEventState);
  BINDSC(AddClientEventToNotificationGroup);
  BINDSC(RemoveClientEvent);
  BINDSC(SetNotificationGroupPriority);
  BINDSC(ClearNotificationGroup);
  BINDSC(RequestNotificationGroup);
  BINDSC(AddToDataDefinition);
  BINDSC(ClearDataDefinition);
  BINDSC(RequestDataOnSimObject);
  BINDSC(RequestDataOnSimObjectType);
  BINDSC(SetDataOnSimObject);
  BINDSC(MapInputEventToClientEvent);
  BINDSC(SetInputGroupPriority);
  BINDSC(RemoveInputEvent);
  BINDSC(ClearInputGroup);
  BINDSC(SetInputGroupState);
  BINDSC(RequestReservedKey);
  BINDSC(SubscribeToSystemEvent);
  BINDSC(UnsubscribeFromSystemEvent);
  BINDSC(WeatherRequestInterpolatedObservation);
  BINDSC(WeatherRequestObservationAtStation);
  BINDSC(WeatherRequestObservationAtNearestStation);
  BINDSC(WeatherCreateStation);
  BINDSC(WeatherRemoveStation);
  BINDSC(WeatherSetObservation);
  BINDSC(WeatherSetModeServer);
  BINDSC(WeatherSetModeTheme);
  BINDSC(WeatherSetModeGlobal);
  BINDSC(WeatherSetModeCustom);
  BINDSC(WeatherSetDynamicUpdateRate);
  BINDSC(WeatherRequestCloudState);
  BINDSC(WeatherCreateThermal);
  BINDSC(WeatherRemoveThermal);
  BINDSC(AICreateParkedATCAircraft);
  BINDSC(AICreateEnrouteATCAircraft);
  BINDSC(AICreateNonATCAircraft);
  BINDSC(AICreateSimulatedObject);
  BINDSC(AIReleaseControl);
  BINDSC(AIRemoveObject);
  BINDSC(AISetAircraftFlightPlan);
  BINDSC(ExecuteMissionAction);
  BINDSC(CompleteCustomMissionAction);
  BINDSC(RetrieveString);
  BINDSC(GetLastSentPacketID);
  BINDSC(CallDispatch);
  BINDSC(GetNextDispatch);
  BINDSC(RequestResponseTimes);
  BINDSC(InsertString);
  BINDSC(CameraSetRelative6DOF);
  BINDSC(MenuAddItem);
  BINDSC(MenuDeleteItem);
  BINDSC(MenuAddSubItem);
  BINDSC(MenuDeleteSubItem);
  BINDSC(RequestSystemState);
  BINDSC(SetSystemState);
  BINDSC(MapClientDataNameToID);
  BINDSC(CreateClientData);
  BINDSC(AddToClientDataDefinition);
  BINDSC(ClearClientDataDefinition);
  BINDSC(RequestClientData);
  BINDSC(SetClientData);
  BINDSC(FlightLoad);
  BINDSC(FlightSave);
  BINDSC(FlightPlanLoad);
  BINDSC(Text);
  BINDSC(SubscribeToFacilities);
  BINDSC(UnsubscribeToFacilities);
  BINDSC(RequestFacilitiesList);

  qDebug() << Q_FUNC_INFO << "done";

  return !error;
}

HRESULT SimConnectApi::Open(LPCSTR szName, HWND hWnd, DWORD UserEventWin32, HANDLE hEventHandle, DWORD ConfigIndex)
{
  Close();
  return SC_Open(&hSimConnect, szName, hWnd, UserEventWin32, hEventHandle, ConfigIndex);
}

HRESULT SimConnectApi::Close()
{
  if(hSimConnect != NULL)
  {
    HRESULT hr = SC_Close(hSimConnect);
    hSimConnect = NULL;
    return hr;
  }
  else
    return S_OK;
}

HRESULT SimConnectApi::MapClientEventToSimEvent(SIMCONNECT_CLIENT_EVENT_ID EventID, const char *EventName)
{
  return SC_MapClientEventToSimEvent(hSimConnect, EventID, EventName);
}

HRESULT SimConnectApi::TransmitClientEvent(SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                           DWORD dwData, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                           SIMCONNECT_EVENT_FLAG Flags)
{
  return SC_TransmitClientEvent(hSimConnect, ObjectID, EventID, dwData, GroupID, Flags);
}

HRESULT SimConnectApi::SetSystemEventState(SIMCONNECT_CLIENT_EVENT_ID EventID, SIMCONNECT_STATE dwState)
{
  return SC_SetSystemEventState(hSimConnect, EventID, dwState);
}

HRESULT SimConnectApi::AddClientEventToNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                         SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                         WINBOOL bMaskable)
{
  return SC_AddClientEventToNotificationGroup(hSimConnect, GroupID, EventID, bMaskable);
}

HRESULT SimConnectApi::RemoveClientEvent(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, SIMCONNECT_CLIENT_EVENT_ID EventID)
{
  return SC_RemoveClientEvent(hSimConnect, GroupID, EventID);
}

HRESULT SimConnectApi::SetNotificationGroupPriority(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, DWORD uPriority)
{
  return SC_SetNotificationGroupPriority(hSimConnect, GroupID, uPriority);
}

HRESULT SimConnectApi::ClearNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID)
{
  return SC_ClearNotificationGroup(hSimConnect, GroupID);
}

HRESULT SimConnectApi::RequestNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, DWORD dwReserved, DWORD Flags)
{
  return SC_RequestNotificationGroup(hSimConnect, GroupID, dwReserved, Flags);
}

HRESULT SimConnectApi::AddToDataDefinition(SIMCONNECT_DATA_DEFINITION_ID DefineID, const char *DatumName,
                                           const char *UnitsName, SIMCONNECT_DATATYPE DatumType, float fEpsilon,
                                           DWORD DatumID)
{
  return SC_AddToDataDefinition(hSimConnect, DefineID, DatumName, UnitsName, DatumType, fEpsilon, DatumID);
}

HRESULT SimConnectApi::ClearDataDefinition(SIMCONNECT_DATA_DEFINITION_ID DefineID)
{
  return SC_ClearDataDefinition(hSimConnect, DefineID);
}

HRESULT SimConnectApi::RequestDataOnSimObject(SIMCONNECT_DATA_REQUEST_ID RequestID,
                                              SIMCONNECT_DATA_DEFINITION_ID DefineID, SIMCONNECT_OBJECT_ID ObjectID,
                                              SIMCONNECT_PERIOD Period,
                                              SIMCONNECT_DATA_REQUEST_FLAG Flags, DWORD origin, DWORD interval,
                                              DWORD limit)
{
  return SC_RequestDataOnSimObject(hSimConnect, RequestID, DefineID, ObjectID, Period, Flags, origin, interval, limit);
}

HRESULT SimConnectApi::RequestDataOnSimObjectType(SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                  SIMCONNECT_DATA_DEFINITION_ID DefineID, DWORD dwRadiusMeters,
                                                  SIMCONNECT_SIMOBJECT_TYPE type)
{
  return SC_RequestDataOnSimObjectType(hSimConnect, RequestID, DefineID, dwRadiusMeters, type);
}

HRESULT SimConnectApi::SetDataOnSimObject(SIMCONNECT_DATA_DEFINITION_ID DefineID, SIMCONNECT_OBJECT_ID ObjectID,
                                          SIMCONNECT_DATA_SET_FLAG Flags, DWORD ArrayCount, DWORD cbUnitSize,
                                          void *pDataSet)
{
  return SC_SetDataOnSimObject(hSimConnect, DefineID, ObjectID, Flags, ArrayCount, cbUnitSize, pDataSet);
}

HRESULT SimConnectApi::MapInputEventToClientEvent(SIMCONNECT_INPUT_GROUP_ID GroupID, const char *szInputDefinition,
                                                  SIMCONNECT_CLIENT_EVENT_ID DownEventID, DWORD DownValue,
                                                  SIMCONNECT_CLIENT_EVENT_ID UpEventID, DWORD UpValue,
                                                  WINBOOL bMaskable)
{
  return SC_MapInputEventToClientEvent(hSimConnect, GroupID, szInputDefinition, DownEventID, DownValue, UpEventID,
                                       UpValue,
                                       bMaskable);
}

HRESULT SimConnectApi::SetInputGroupPriority(SIMCONNECT_INPUT_GROUP_ID GroupID, DWORD uPriority)
{
  return SC_SetInputGroupPriority(hSimConnect, GroupID, uPriority);
}

HRESULT SimConnectApi::RemoveInputEvent(SIMCONNECT_INPUT_GROUP_ID GroupID, const char *szInputDefinition)
{
  return SC_RemoveInputEvent(hSimConnect, GroupID, szInputDefinition);
}

HRESULT SimConnectApi::ClearInputGroup(SIMCONNECT_INPUT_GROUP_ID GroupID)
{
  return SC_ClearInputGroup(hSimConnect, GroupID);
}

HRESULT SimConnectApi::SetInputGroupState(SIMCONNECT_INPUT_GROUP_ID GroupID, DWORD dwState)
{
  return SC_SetInputGroupState(hSimConnect, GroupID, dwState);
}

HRESULT SimConnectApi::RequestReservedKey(SIMCONNECT_CLIENT_EVENT_ID EventID, const char *szKeyChoice1,
                                          const char *szKeyChoice2,
                                          const char *szKeyChoice3)
{
  return SC_RequestReservedKey(hSimConnect, EventID, szKeyChoice1, szKeyChoice2, szKeyChoice3);
}

HRESULT SimConnectApi::SubscribeToSystemEvent(SIMCONNECT_CLIENT_EVENT_ID EventID, const char *SystemEventName)
{
  return SC_SubscribeToSystemEvent(hSimConnect, EventID, SystemEventName);
}

HRESULT SimConnectApi::UnsubscribeFromSystemEvent(SIMCONNECT_CLIENT_EVENT_ID EventID)
{
  return SC_UnsubscribeFromSystemEvent(hSimConnect, EventID);
}

HRESULT SimConnectApi::WeatherRequestInterpolatedObservation(SIMCONNECT_DATA_REQUEST_ID RequestID, float lat, float lon,
                                                             float alt)
{
  return SC_WeatherRequestInterpolatedObservation(hSimConnect, RequestID, lat, lon, alt);
}

HRESULT SimConnectApi::WeatherRequestObservationAtStation(SIMCONNECT_DATA_REQUEST_ID RequestID, const char *szICAO)
{
  return SC_WeatherRequestObservationAtStation(hSimConnect, RequestID, szICAO);
}

HRESULT SimConnectApi::WeatherRequestObservationAtNearestStation(SIMCONNECT_DATA_REQUEST_ID RequestID, float lat,
                                                                 float lon)
{
  return SC_WeatherRequestObservationAtNearestStation(hSimConnect, RequestID, lat, lon);
}

HRESULT SimConnectApi::WeatherCreateStation(SIMCONNECT_DATA_REQUEST_ID RequestID, const char *szICAO,
                                            const char *szName, float lat, float lon,
                                            float alt)
{
  return SC_WeatherCreateStation(hSimConnect, RequestID, szICAO, szName, lat, lon, alt);
}

HRESULT SimConnectApi::WeatherRemoveStation(SIMCONNECT_DATA_REQUEST_ID RequestID, const char *szICAO)
{
  return SC_WeatherRemoveStation(hSimConnect, RequestID, szICAO);
}

HRESULT SimConnectApi::WeatherSetObservation(DWORD Seconds, const char *szMETAR)
{
  return SC_WeatherSetObservation(hSimConnect, Seconds, szMETAR);
}

HRESULT SimConnectApi::WeatherSetModeServer(DWORD dwPort, DWORD dwSeconds)
{
  return SC_WeatherSetModeServer(hSimConnect, dwPort, dwSeconds);
}

HRESULT SimConnectApi::WeatherSetModeTheme(const char *szThemeName)
{
  return SC_WeatherSetModeTheme(hSimConnect, szThemeName);
}

HRESULT SimConnectApi::WeatherSetModeGlobal()
{
  return SC_WeatherSetModeGlobal(hSimConnect);
}

HRESULT SimConnectApi::WeatherSetModeCustom()
{
  return SC_WeatherSetModeCustom(hSimConnect);
}

HRESULT SimConnectApi::WeatherSetDynamicUpdateRate(DWORD dwRate)
{
  return SC_WeatherSetDynamicUpdateRate(hSimConnect, dwRate);
}

HRESULT SimConnectApi::WeatherRequestCloudState(SIMCONNECT_DATA_REQUEST_ID RequestID, float minLat, float minLon,
                                                float minAlt, float maxLat, float maxLon, float maxAlt,
                                                DWORD dwFlags)
{
  return SC_WeatherRequestCloudState(hSimConnect, RequestID, minLat, minLon, minAlt, maxLat, maxLon, maxAlt, dwFlags);
}

HRESULT SimConnectApi::WeatherCreateThermal(SIMCONNECT_DATA_REQUEST_ID RequestID, float lat, float lon, float alt,
                                            float radius, float height, float coreRate, float coreTurbulence,
                                            float sinkRate, float sinkTurbulence, float coreSize,
                                            float coreTransitionSize, float sinkLayerSize,
                                            float sinkTransitionSize)
{
  return SC_WeatherCreateThermal(hSimConnect, RequestID, lat, lon, alt, radius, height, coreRate, coreTurbulence,
                                 sinkRate, sinkTurbulence, coreSize, coreTransitionSize, sinkLayerSize,
                                 sinkTransitionSize);
}

HRESULT SimConnectApi::WeatherRemoveThermal(SIMCONNECT_OBJECT_ID ObjectID)
{
  return SC_WeatherRemoveThermal(hSimConnect, ObjectID);
}

HRESULT SimConnectApi::AICreateParkedATCAircraft(const char *szContainerTitle, const char *szTailNumber,
                                                 const char *szAirportID,
                                                 SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return SC_AICreateParkedATCAircraft(hSimConnect, szContainerTitle, szTailNumber, szAirportID, RequestID);
}

HRESULT SimConnectApi::AICreateEnrouteATCAircraft(const char *szContainerTitle, const char *szTailNumber,
                                                  int iFlightNumber, const char *szFlightPlanPath,
                                                  double dFlightPlanPosition, WINBOOL bTouchAndGo,
                                                  SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return SC_AICreateEnrouteATCAircraft(hSimConnect, szContainerTitle, szTailNumber, iFlightNumber, szFlightPlanPath,
                                       dFlightPlanPosition, bTouchAndGo,
                                       RequestID);
}

HRESULT SimConnectApi::AICreateNonATCAircraft(const char *szContainerTitle, const char *szTailNumber,
                                              SIMCONNECT_DATA_INITPOSITION InitPos,
                                              SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return SC_AICreateNonATCAircraft(hSimConnect, szContainerTitle, szTailNumber, InitPos, RequestID);
}

HRESULT SimConnectApi::AICreateSimulatedObject(const char *szContainerTitle, SIMCONNECT_DATA_INITPOSITION InitPos,
                                               SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return SC_AICreateSimulatedObject(hSimConnect, szContainerTitle, InitPos, RequestID);
}

HRESULT SimConnectApi::AIReleaseControl(SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return SC_AIReleaseControl(hSimConnect, ObjectID, RequestID);
}

HRESULT SimConnectApi::AIRemoveObject(SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return SC_AIRemoveObject(hSimConnect, ObjectID, RequestID);
}

HRESULT SimConnectApi::AISetAircraftFlightPlan(SIMCONNECT_OBJECT_ID ObjectID, const char *szFlightPlanPath,
                                               SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return SC_AISetAircraftFlightPlan(hSimConnect, ObjectID, szFlightPlanPath, RequestID);
}

HRESULT SimConnectApi::ExecuteMissionAction(const GUID guidInstanceId)
{
  return SC_ExecuteMissionAction(hSimConnect, guidInstanceId);
}

HRESULT SimConnectApi::CompleteCustomMissionAction(const GUID guidInstanceId)
{
  return SC_CompleteCustomMissionAction(hSimConnect, guidInstanceId);
}

HRESULT SimConnectApi::RetrieveString(SIMCONNECT_RECV *pData, DWORD cbData, void *pStringV, char **pszString,
                                      DWORD *pcbString)
{
  return SC_RetrieveString(pData, cbData, pStringV, pszString, pcbString);
}

HRESULT SimConnectApi::GetLastSentPacketID(DWORD *pdwError)
{
  return SC_GetLastSentPacketID(hSimConnect, pdwError);
}

HRESULT SimConnectApi::CallDispatch(DispatchProc pfcnDispatch, void *pContext)
{
  return SC_CallDispatch(hSimConnect, pfcnDispatch, pContext);
}

HRESULT SimConnectApi::GetNextDispatch(SIMCONNECT_RECV **ppData, DWORD *pcbData)
{
  return SC_GetNextDispatch(hSimConnect, ppData, pcbData);
}

HRESULT SimConnectApi::RequestResponseTimes(DWORD nCount, float *fElapsedSeconds)
{
  return SC_RequestResponseTimes(hSimConnect, nCount, fElapsedSeconds);
}

HRESULT SimConnectApi::InsertString(char *pDest, DWORD cbDest, void **ppEnd, DWORD *pcbStringV, const char *pSource)
{
  return SC_InsertString(pDest, cbDest, ppEnd, pcbStringV, pSource);
}

HRESULT SimConnectApi::CameraSetRelative6DOF(float fDeltaX, float fDeltaY, float fDeltaZ, float fPitchDeg,
                                             float fBankDeg,
                                             float fHeadingDeg)
{
  return SC_CameraSetRelative6DOF(hSimConnect, fDeltaX, fDeltaY, fDeltaZ, fPitchDeg, fBankDeg, fHeadingDeg);
}

HRESULT SimConnectApi::MenuAddItem(const char *szMenuItem, SIMCONNECT_CLIENT_EVENT_ID MenuEventID, DWORD dwData)
{
  return SC_MenuAddItem(hSimConnect, szMenuItem, MenuEventID, dwData);
}

HRESULT SimConnectApi::MenuDeleteItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID)
{
  return SC_MenuDeleteItem(hSimConnect, MenuEventID);
}

HRESULT SimConnectApi::MenuAddSubItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID, const char *szMenuItem,
                                      SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID,
                                      DWORD dwData)
{
  return SC_MenuAddSubItem(hSimConnect, MenuEventID, szMenuItem, SubMenuEventID, dwData);
}

HRESULT SimConnectApi::MenuDeleteSubItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID,
                                         const SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID)
{
  return SC_MenuDeleteSubItem(hSimConnect, MenuEventID, SubMenuEventID);
}

HRESULT SimConnectApi::RequestSystemState(SIMCONNECT_DATA_REQUEST_ID RequestID, const char *szState)
{
  return SC_RequestSystemState(hSimConnect, RequestID, szState);
}

HRESULT SimConnectApi::SetSystemState(const char *szState, DWORD dwInteger, float fFloat, const char *szString)
{
  return SC_SetSystemState(hSimConnect, szState, dwInteger, fFloat, szString);
}

HRESULT SimConnectApi::MapClientDataNameToID(const char *szClientDataName, SIMCONNECT_CLIENT_DATA_ID ClientDataID)
{
  return SC_MapClientDataNameToID(hSimConnect, szClientDataName, ClientDataID);
}

HRESULT SimConnectApi::CreateClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID, DWORD dwSize,
                                        SIMCONNECT_CREATE_CLIENT_DATA_FLAG Flags)
{
  return SC_CreateClientData(hSimConnect, ClientDataID, dwSize, Flags);
}

HRESULT SimConnectApi::AddToClientDataDefinition(SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID, DWORD dwOffset,
                                                 DWORD dwSizeOrType, float fEpsilon,
                                                 DWORD DatumID)
{
  return SC_AddToClientDataDefinition(hSimConnect, DefineID, dwOffset, dwSizeOrType, fEpsilon, DatumID);
}

HRESULT SimConnectApi::ClearClientDataDefinition(SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID)
{
  return SC_ClearClientDataDefinition(hSimConnect, DefineID);
}

HRESULT SimConnectApi::RequestClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                         SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                         SIMCONNECT_CLIENT_DATA_PERIOD Period,
                                         SIMCONNECT_CLIENT_DATA_REQUEST_FLAG Flags, DWORD origin, DWORD interval,
                                         DWORD limit)
{
  return SC_RequestClientData(hSimConnect, ClientDataID, RequestID, DefineID, Period, Flags, origin, interval, limit);
}

HRESULT SimConnectApi::SetClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                     SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                     SIMCONNECT_CLIENT_DATA_SET_FLAG Flags, DWORD dwReserved, DWORD cbUnitSize,
                                     void *pDataSet)
{
  return SC_SetClientData(hSimConnect, ClientDataID, DefineID, Flags, dwReserved, cbUnitSize, pDataSet);
}

HRESULT SimConnectApi::FlightLoad(const char *szFileName)
{
  return SC_FlightLoad(hSimConnect, szFileName);
}

HRESULT SimConnectApi::FlightSave(const char *szFileName, const char *szTitle, const char *szDescription, DWORD Flags)
{
  return SC_FlightSave(hSimConnect, szFileName, szTitle, szDescription, Flags);
}

HRESULT SimConnectApi::FlightPlanLoad(const char *szFileName)
{
  return SC_FlightPlanLoad(hSimConnect, szFileName);
}

HRESULT SimConnectApi::Text(SIMCONNECT_TEXT_TYPE type, float fTimeSeconds, SIMCONNECT_CLIENT_EVENT_ID EventID,
                            DWORD cbUnitSize, void *pDataSet)
{
  return SC_Text(hSimConnect, type, fTimeSeconds, EventID, cbUnitSize, pDataSet);
}

HRESULT SimConnectApi::SubscribeToFacilities(SIMCONNECT_FACILITY_LIST_TYPE type, SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return SC_SubscribeToFacilities(hSimConnect, type, RequestID);
}

HRESULT SimConnectApi::UnsubscribeToFacilities(SIMCONNECT_FACILITY_LIST_TYPE type)
{
  return SC_UnsubscribeToFacilities(hSimConnect, type);
}

HRESULT SimConnectApi::RequestFacilitiesList(SIMCONNECT_FACILITY_LIST_TYPE type, SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return SC_RequestFacilitiesList(hSimConnect, type, RequestID);
}

} // namespace sc
} // namespace fs
} // namespace atools