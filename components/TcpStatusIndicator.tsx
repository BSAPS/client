"use client"

// TCP 연결 상태 표시 컴포넌트
import { useTcpService } from "../hooks/useTcpService"

interface TcpStatusIndicatorProps {
  className?: string
}

export function TcpStatusIndicator({ className = "" }: TcpStatusIndicatorProps) {
  const { isConnected, error } = useTcpService()

  return (
    <div className={`flex items-center gap-2 ${className}`}>
      {/* 연결 상태 인디케이터 */}
      <div className={`w-3 h-3 rounded-full ${isConnected ? "bg-green-500" : "bg-red-500"} animate-pulse`} />

      {/* 상태 텍스트 */}
      <span className="text-sm font-medium">TCP: {isConnected ? "연결됨" : "연결 안됨"}</span>

      {/* 에러 메시지 */}
      {error && <span className="text-xs text-red-500 ml-2">{error}</span>}
    </div>
  )
}
