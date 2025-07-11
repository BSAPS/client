"use client"

// TCP 서비스를 위한 커스텀 훅
import { useState, useEffect, useCallback } from "react"
import { tcpService } from "../services/tcpService"
import type { TcpPacket, ImageTextData } from "../types/tcp"
import type { LineCoordinate } from "../types/system"

export function useTcpService() {
  const [isConnected, setIsConnected] = useState(false)
  const [lastMessage, setLastMessage] = useState<TcpPacket | null>(null)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    // 연결 상태 변경 콜백 설정
    tcpService.setConnectionChangeCallback((connected) => {
      setIsConnected(connected)
      if (!connected) {
        setError("서버 연결이 끊어졌습니다")
      } else {
        setError(null)
      }
    })

    // 메시지 수신 콜백 설정
    tcpService.setMessageReceivedCallback((packet) => {
      setLastMessage(packet)
    })

    return () => {
      tcpService.disconnect()
    }
  }, [])

  // 서버 연결
  const connect = useCallback(async (host: string, port: number) => {
    try {
      setError(null)

      // 연결 테스트
      const testResult = tcpService.testConnection(host, port)
      console.log("연결 테스트:", testResult)

      const success = await tcpService.connect(host, port)
      if (!success) {
        const isSecure = window.location.protocol === "https:"
        const suggestion = isSecure
          ? "HTTPS 환경에서는 WSS(보안 WebSocket) 연결이 필요합니다. 서버가 WSS를 지원하는지 확인하세요."
          : "서버 주소와 포트를 확인하고, 서버가 실행 중인지 확인하세요."

        setError(`서버 연결에 실패했습니다. ${suggestion}`)
      }
      return success
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : "알 수 없는 오류"
      setError(`연결 중 오류가 발생했습니다: ${errorMessage}`)
      return false
    }
  }, [])

  // 연결 해제
  const disconnect = useCallback(() => {
    tcpService.disconnect()
  }, [])

  // 이미지&텍스트 데이터 전송
  const sendImageTextData = useCallback(async (data: ImageTextData) => {
    try {
      const success = await tcpService.sendImageTextData(data)
      if (!success) {
        setError("이미지&텍스트 데이터 전송 실패")
      }
      return success
    } catch (err) {
      setError("데이터 전송 중 오류 발생")
      return false
    }
  }, [])

  // 라인 좌표 전송
  const sendLineCoordinates = useCallback(async (coordinates: LineCoordinate) => {
    try {
      const success = await tcpService.sendLineCoordinates(coordinates)
      if (!success) {
        setError("라인 좌표 전송 실패")
      }
      return success
    } catch (err) {
      setError("좌표 전송 중 오류 발생")
      return false
    }
  }, [])

  // 라인 동기화 요청
  const requestLineSync = useCallback(async () => {
    try {
      const success = await tcpService.requestLineSync()
      if (!success) {
        setError("라인 동기화 요청 실패")
      }
      return success
    } catch (err) {
      setError("동기화 요청 중 오류 발생")
      return false
    }
  }, [])

  return {
    isConnected,
    lastMessage,
    error,
    connect,
    disconnect,
    sendImageTextData,
    sendLineCoordinates,
    requestLineSync,
  }
}
