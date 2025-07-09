"use client"

// RTSP 스트림 관리를 위한 커스텀 훅
import { useState, useEffect, useRef } from "react"

export function useRTSPStream(rtspUrl: string) {
  const [isConnected, setIsConnected] = useState(false) // 연결 상태
  const [error, setError] = useState<string | null>(null) // 에러 상태
  const videoRef = useRef<HTMLVideoElement>(null) // 비디오 엘리먼트 참조

  useEffect(() => {
    if (!rtspUrl || !videoRef.current) return

    const video = videoRef.current

    // RTSP 스트림 연결 시도
    const connectStream = async () => {
      try {
        setError(null)

        // WebRTC를 통한 RTSP 스트림 연결 (실제 구현에서는 WebRTC 라이브러리 사용)
        // 여기서는 시뮬레이션을 위해 placeholder 사용
        video.src = rtspUrl

        video.onloadstart = () => {
          console.log("RTSP 스트림 로딩 시작")
        }

        video.oncanplay = () => {
          setIsConnected(true)
          console.log("RTSP 스트림 연결 성공")
        }

        video.onerror = (e) => {
          setError("RTSP 스트림 연결 실패")
          setIsConnected(false)
          console.error("RTSP 스트림 에러:", e)
        }

        await video.play()
      } catch (err) {
        setError("스트림 연결 중 오류 발생")
        setIsConnected(false)
        console.error("스트림 연결 에러:", err)
      }
    }

    connectStream()

    // 컴포넌트 언마운트 시 정리
    return () => {
      if (video) {
        video.pause()
        video.src = ""
      }
    }
  }, [rtspUrl])

  return { videoRef, isConnected, error }
}
