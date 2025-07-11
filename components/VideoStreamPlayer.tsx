"use client"
import { useRTSPStream } from "../hooks/useRTSPStream"

interface VideoStreamPlayerProps {
  rtspUrl: string // RTSP 스트림 URL
  onClick?: () => void // 클릭 이벤트 핸들러
  className?: string // CSS 클래스명
}

export function VideoStreamPlayer({ rtspUrl, onClick, className }: VideoStreamPlayerProps) {
  const { videoRef, isConnected, error } = useRTSPStream(rtspUrl)

  return (
    <div className={`relative ${className}`}>
      {/* 비디오 엘리먼트 */}
      <video
        ref={videoRef}
        className="w-full h-full object-cover rounded-lg cursor-pointer"
        onClick={onClick}
        muted
        playsInline
      />

      {/* 연결 상태 표시 */}
      <div className="absolute top-2 left-2 flex items-center gap-2">
        <div className={`w-3 h-3 rounded-full ${isConnected ? "bg-green-500" : "bg-red-500"}`} />
        <span className="text-white text-sm bg-black bg-opacity-50 px-2 py-1 rounded">
          {isConnected ? "LIVE" : "OFFLINE"}
        </span>
      </div>

      {/* 에러 메시지 표시 */}
      {error && (
        <div className="absolute inset-0 flex items-center justify-center bg-black bg-opacity-75">
          <div className="text-white text-center">
            <p className="text-lg mb-2">연결 오류</p>
            <p className="text-sm">{error}</p>
          </div>
        </div>
      )}

      {/* 로딩 상태 표시 */}
      {!isConnected && !error && (
        <div className="absolute inset-0 flex items-center justify-center bg-gray-200">
          <div className="text-gray-500 text-center">
            <div className="animate-spin w-8 h-8 border-4 border-blue-500 border-t-transparent rounded-full mx-auto mb-2" />
            <p>스트림 연결 중...</p>
          </div>
        </div>
      )}
    </div>
  )
}
