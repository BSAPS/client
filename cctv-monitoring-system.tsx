"use client"

import { useState, useEffect } from "react"
import { Globe, Play, Video, Camera, Settings } from "lucide-react"
import { Button } from "@/components/ui/button"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import { VideoStreamPlayer } from "./components/VideoStreamPlayer"
import { LineDrawingModal } from "./components/LineDrawingModal"
import { useTcpService } from "./hooks/useTcpService"
import { logService } from "./services/logService"
import type { LogData, LineCoordinate, ServerConfig } from "./types/system"

export default function Component() {
  // 기본 UI 상태
  const [warningStates, setWarningStates] = useState([false, false, false, false]) // 경고 상태 배열
  const [showNetworkModal, setShowNetworkModal] = useState(false) // 네트워크 모달 표시 상태
  const [activeTab, setActiveTab] = useState<"live" | "captured">("live") // 활성 탭 상태
  const [connectionStatus, setConnectionStatus] = useState<"none" | "connected" | "disconnected">("none") // 연결 상태

  // 서버 설정 상태
  const [serverConfig, setServerConfig] = useState<ServerConfig>({
    rtspUrl: "rtsp://192.168.1.100:554/stream", // 기본 RTSP URL
    tcpHost: "192.168.1.100", // 기본 TCP 호스트
    tcpPort: 8080, // 기본 TCP 포트
  })

  // 비디오 스트림 상태
  const [showLineDrawing, setShowLineDrawing] = useState(false) // 기준선 그리기 모달 표시 상태
  const [currentLine, setCurrentLine] = useState<LineCoordinate | null>(null) // 현재 설정된 기준선

  // 로그 데이터 상태
  const [logs, setLogs] = useState<LogData[]>([]) // 로그 데이터 배열
  const [selectedLog, setSelectedLog] = useState<LogData | null>(null) // 선택된 로그
  const [selectedDate, setSelectedDate] = useState("") // 선택된 날짜
  const [selectedTime, setSelectedTime] = useState("") // 선택된 시간

  // TCP 서비스 훅 사용
  const {
    isConnected: tcpConnected,
    connect,
    disconnect,
    sendLineCoordinates,
    sendImageTextData,
    requestLineSync,
    lastMessage,
    error: tcpError,
  } = useTcpService()

  // 컴포넌트 마운트 시 로그 데이터 로드
  useEffect(() => {
    const loadLogs = async () => {
      const logData = await logService.fetchLogs()
      setLogs(logData)

      // 첫 번째 로그를 기본 선택
      if (logData.length > 0) {
        setSelectedLog(logData[0])
        setSelectedDate(logData[0].date)
        setSelectedTime(logData[0].time)
      }
    }

    loadLogs()
  }, [])

  // 경고 상태 토글 함수
  const toggleWarning = (index: number) => {
    setWarningStates((prev) => prev.map((state, i) => (i === index ? !state : state)))
  }

  // 비디오 클릭 시 기준선 그리기 모달 열기
  const handleVideoClick = () => {
    if (connectionStatus === "connected") {
      setShowLineDrawing(true)
    }
  }

  // 기준선 그리기 완료 처리
  const handleLineDrawn = async (coordinates: LineCoordinate) => {
    setCurrentLine(coordinates)

    // TCP를 통해 서버로 좌표 전송
    const success = await sendLineCoordinates(coordinates)
    if (success) {
      console.log("기준선 좌표 전송 완료:", coordinates)
    }

    setShowLineDrawing(false)
  }

  // 날짜 변경 처리
  const handleDateChange = (date: string) => {
    setSelectedDate(date)
    const filteredLogs = logService.getLogsByDate(date)
    if (filteredLogs.length > 0) {
      setSelectedLog(filteredLogs[0])
      setSelectedTime(filteredLogs[0].time)
    }
  }

  // 시간 변경 처리
  const handleTimeChange = (time: string) => {
    setSelectedTime(time)
    const filteredLogs = logService.getLogsByTime(time)
    if (filteredLogs.length > 0) {
      setSelectedLog(filteredLogs[0])
    }
  }

  // TCP 메시지 수신 처리
  useEffect(() => {
    if (lastMessage) {
      console.log("TCP 메시지 수신:", lastMessage)
      // 필요에 따라 메시지 타입별 처리 로직 추가
    }
  }, [lastMessage])

  return (
    <div className="min-h-screen bg-[#292d41] p-6">
      {/* TCP 에러 표시 */}
      {tcpError && (
        <div className="fixed top-4 right-4 bg-red-500 text-white p-3 rounded-lg shadow-lg z-50">{tcpError}</div>
      )}
      {/* 헤더 영역 */}
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-[#cf5e2d] text-2xl font-semibold">CCTV Remote Monitoring System</h1>
        <button onClick={() => setShowNetworkModal(true)} className="text-white hover:text-gray-300 transition-colors">
          <Globe className="w-8 h-8" />
        </button>
      </div>

      <div className="flex gap-6">
        {/* 메인 콘텐츠 영역 */}
        <div className="flex-1">
          {/* 비디오/이미지 표시 영역 */}
          <div className="relative mb-4">
            {activeTab === "live" ? (
              // 라이브 비디오 스트림 표시
              <div className="bg-[#d9d9d9] aspect-video rounded-lg overflow-hidden">
                {connectionStatus === "connected" ? (
                  <VideoStreamPlayer
                    rtspUrl={serverConfig.rtspUrl}
                    onClick={handleVideoClick}
                    className="w-full h-full"
                  />
                ) : (
                  <div className="w-full h-full flex items-center justify-center text-gray-500 text-lg">
                    서버에 연결하여 비디오 스트림을 시작하세요
                  </div>
                )}
              </div>
            ) : (
              // 캡처된 이미지 표시
              <div className="bg-[#d9d9d9] aspect-video rounded-lg overflow-hidden">
                {selectedLog ? (
                  <img
                    src={selectedLog.imageUrl || "/placeholder.svg"}
                    alt="Captured Image"
                    className="w-full h-full object-cover"
                  />
                ) : (
                  <div className="w-full h-full flex items-center justify-center text-gray-500 text-lg">
                    캡처된 이미지가 없습니다
                  </div>
                )}
              </div>
            )}

            {/* 스트리밍 버튼 - 라이브 탭에서만 표시 */}
            {activeTab === "live" && (
              <div className="mt-4">
                <Button
                  className={`px-6 py-2 rounded-md text-white ${
                    connectionStatus === "connected"
                      ? "bg-green-500 hover:bg-green-600"
                      : "bg-[#409cff] hover:bg-[#409cff]/90"
                  }`}
                  disabled={connectionStatus === "none"}
                >
                  <Play className="w-4 h-4 mr-2 fill-white" />
                  {connectionStatus === "connected" ? "스트리밍 중" : "스트리밍"}
                </Button>
              </div>
            )}
          </div>

          {/* 하단 탭 영역 */}
          <div className="bg-white rounded-lg p-4">
            <div className="flex">
              <button
                onClick={() => setActiveTab("live")}
                className={`flex-1 text-center py-3 ${activeTab === "live" ? "border-b-2 border-[#6750a4]" : ""}`}
              >
                <div
                  className={`flex items-center justify-center gap-2 ${
                    activeTab === "live" ? "text-[#6750a4]" : "text-gray-500"
                  }`}
                >
                  <Video className="w-5 h-5" />
                  <span className="font-medium">Live Video Stream</span>
                </div>
              </button>
              <button
                onClick={() => setActiveTab("captured")}
                className={`flex-1 text-center py-3 ${activeTab === "captured" ? "border-b-2 border-[#6750a4]" : ""}`}
              >
                <div
                  className={`flex items-center justify-center gap-2 ${
                    activeTab === "captured" ? "text-[#6750a4]" : "text-gray-500"
                  }`}
                >
                  <Camera className="w-5 h-5" />
                  <span className="font-medium">Captured Image</span>
                </div>
              </button>
            </div>
          </div>
        </div>

        {/* 우측 사이드바 */}
        <div className="w-80 space-y-4">
          {activeTab === "live" ? (
            // 라이브 비디오 스트림 사이드바
            <>
              {/* 수동 제어 드롭다운 */}
              <Select defaultValue="manual">
                <SelectTrigger className="bg-[#6750a4] text-white border-none hover:bg-[#6750a4]/90">
                  <div className="flex items-center gap-2">
                    <Settings className="w-4 h-4" />
                    <SelectValue />
                  </div>
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="manual">Manual</SelectItem>
                  <SelectItem value="automatic">Automatic</SelectItem>
                </SelectContent>
              </Select>

              {/* 경고 카드들 */}
              <div className="space-y-3">
                {[1, 2, 3, 4].map((num, index) => (
                  <button
                    key={num}
                    onClick={() => toggleWarning(index)}
                    className={`w-full p-4 rounded-lg text-center font-medium transition-colors duration-200 ${
                      warningStates[index] ? "bg-[#cf5e2d] text-white" : "bg-[#b4afb9] text-white"
                    }`}
                  >
                    Warning {num} {warningStates[index] ? "(ON)" : "(OFF)"}
                  </button>
                ))}
              </div>

              {/* 기준선 정보 표시 */}
              {currentLine && (
                <div className="bg-white p-4 rounded-lg">
                  <h3 className="font-medium mb-2">설정된 기준선</h3>
                  <p className="text-sm text-gray-600">
                    시작: ({currentLine.x1}, {currentLine.y1})
                  </p>
                  <p className="text-sm text-gray-600">
                    끝: ({currentLine.x2}, {currentLine.y2})
                  </p>
                </div>
              )}
            </>
          ) : (
            // 캡처된 이미지 사이드바
            <div className="space-y-6">
              {/* 날짜 입력 */}
              <div>
                <label className="block text-white text-xl font-medium mb-3">Date</label>
                <input
                  type="date"
                  value={selectedDate}
                  onChange={(e) => handleDateChange(e.target.value)}
                  className="w-full bg-[#d9d9d9] rounded-lg p-4 border border-gray-300 outline-none focus:border-gray-400"
                />
              </div>

              {/* 시간 입력 */}
              <div>
                <label className="block text-white text-xl font-medium mb-3">Time</label>
                <input
                  type="time"
                  value={selectedTime}
                  onChange={(e) => handleTimeChange(e.target.value)}
                  className="w-full bg-[#d9d9d9] rounded-lg p-4 border border-gray-300 outline-none focus:border-gray-400"
                />
              </div>

              {/* 로그 정보 표시 */}
              {selectedLog && (
                <div className="bg-white p-4 rounded-lg">
                  <h3 className="font-medium mb-2">감지 정보</h3>
                  <p className="text-sm text-gray-600 mb-1">
                    타입: {selectedLog.detectionType === "person" ? "보행자" : "차량"}
                  </p>
                  <p className="text-sm text-gray-600">
                    방향: {selectedLog.direction === "incoming" ? "진입" : "진출"}
                  </p>
                </div>
              )}
            </div>
          )}
        </div>
      </div>

      {/* 네트워크 연결 모달 */}
      {showNetworkModal && (
        <div
          className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50"
          onClick={() => setShowNetworkModal(false)}
        >
          <div className="bg-white rounded-2xl p-8 w-96 max-w-md mx-4 shadow-2xl" onClick={(e) => e.stopPropagation()}>
            {/* 모달 핸들 */}
            <div className="w-12 h-1 bg-gray-400 rounded-full mx-auto mb-6"></div>

            <h2 className="text-2xl font-semibold text-gray-800 mb-6">Server Connection Settings</h2>

            {/* 연결 상태 표시 */}
            {tcpConnected && (
              <div className="mb-4 p-3 bg-green-100 border border-green-300 rounded-lg">
                <div className="flex items-center gap-2 text-green-700">
                  <div className="w-2 h-2 bg-green-500 rounded-full"></div>
                  <span className="text-sm font-medium">서버에 연결됨</span>
                </div>
              </div>
            )}

            {/* 연결 프로토콜 정보 */}
            <div className="mb-4 p-3 bg-blue-50 border border-blue-200 rounded-lg">
              <div className="text-sm text-blue-700">
                <div className="font-medium mb-1">연결 정보</div>
                <div>프로토콜: {window.location.protocol === "https:" ? "WSS (보안)" : "WS (일반)"}</div>
                <div>
                  현재 페이지: {window.location.protocol}//{window.location.host}
                </div>
              </div>
            </div>

            {/* 서버 주소 입력 */}
            <div className="mb-6">
              <label className="block text-gray-600 font-medium mb-2">RTSP Stream URL</label>
              <div className="bg-gray-100 rounded-lg p-4 border border-gray-200">
                <input
                  type="text"
                  placeholder="rtsp://192.168.1.100:554/stream"
                  value={serverConfig.rtspUrl}
                  onChange={(e) => setServerConfig((prev) => ({ ...prev, rtspUrl: e.target.value }))}
                  className="w-full bg-transparent outline-none text-gray-700"
                />
              </div>
            </div>

            {/* TCP 호스트 입력 */}
            <div className="mb-4">
              <label className="block text-gray-600 font-medium mb-2">TCP Host</label>
              <input
                type="text"
                placeholder="192.168.1.100"
                value={serverConfig.tcpHost}
                onChange={(e) => setServerConfig((prev) => ({ ...prev, tcpHost: e.target.value }))}
                className="w-full bg-gray-100 rounded-lg p-3 border border-gray-200 outline-none focus:border-gray-400"
              />
            </div>

            {/* TCP 포트 입력 */}
            <div className="mb-6">
              <label className="block text-gray-600 font-medium mb-2">TCP Port</label>
              <input
                type="number"
                placeholder="8080"
                value={serverConfig.tcpPort}
                onChange={(e) =>
                  setServerConfig((prev) => ({ ...prev, tcpPort: Number.parseInt(e.target.value) || 8080 }))
                }
                className="w-full bg-gray-100 rounded-lg p-3 border border-gray-200 outline-none focus:border-gray-400"
              />
            </div>

            {/* HTTPS 환경 안내 */}
            {window.location.protocol === "https:" && (
              <div className="mb-6 p-3 bg-yellow-50 border border-yellow-200 rounded-lg">
                <div className="text-sm text-yellow-700">
                  <div className="font-medium mb-1">⚠️ HTTPS 환경 안내</div>
                  <div>보안 연결(HTTPS)에서는 WSS 프로토콜이 필요합니다. 서버가 WSS를 지원하는지 확인하세요.</div>
                </div>
              </div>
            )}

            {/* 연결/해제 버튼 */}
            <div className="flex gap-4">
              <button
                onClick={async () => {
                  if (tcpConnected) {
                    disconnect()
                    setConnectionStatus("none")
                  } else {
                    const success = await connect(serverConfig.tcpHost, serverConfig.tcpPort)
                    if (success) {
                      setConnectionStatus("connected")
                    }
                  }
                }}
                className={`flex-1 py-3 px-6 rounded-full font-medium transition-all duration-200 flex items-center justify-center gap-2 ${
                  tcpConnected
                    ? "bg-red-500 text-white border-2 border-red-500 hover:bg-red-600"
                    : "bg-blue-500 text-white border-2 border-blue-500 hover:bg-blue-600"
                }`}
              >
                {tcpConnected ? "Disconnect" : "Connect"}
              </button>
              <button
                onClick={() => setShowNetworkModal(false)}
                className="absolute top-4 right-4 text-gray-400 hover:text-gray-600"
              >
                ✕
              </button>
            </div>
          </div>
        </div>
      )}

      {/* 기준선 그리기 모달 */}
      <LineDrawingModal
        isOpen={showLineDrawing}
        onClose={() => setShowLineDrawing(false)}
        rtspUrl={serverConfig.rtspUrl}
        onLineDrawn={handleLineDrawn}
      />
    </div>
  )
}
