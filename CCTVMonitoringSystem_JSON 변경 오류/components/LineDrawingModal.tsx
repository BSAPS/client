"use client"

// 기준선 그리기 모달 컴포넌트
import type React from "react"
import { useState, useRef, useCallback, useEffect } from "react"
import type { LineCoordinate } from "../types/system"

interface LineDrawingModalProps {
  isOpen: boolean // 모달 열림 상태
  onClose: () => void // 모달 닫기 함수
  rtspUrl: string // RTSP 스트림 URL
  onLineDrawn: (coordinates: LineCoordinate) => void // 기준선 그리기 완료 콜백
}

export function LineDrawingModal({ isOpen, onClose, rtspUrl, onLineDrawn }: LineDrawingModalProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null) // 캔버스 참조
  const videoRef = useRef<HTMLVideoElement>(null) // 비디오 참조
  const [isDrawing, setIsDrawing] = useState(false) // 그리기 상태
  const [startPoint, setStartPoint] = useState<{ x: number; y: number } | null>(null) // 시작점
  const [currentLine, setCurrentLine] = useState<LineCoordinate | null>(null) // 현재 그리는 선

  // 캔버스에 비디오 프레임과 선 그리기
  const drawFrame = useCallback(() => {
    const canvas = canvasRef.current
    const video = videoRef.current
    if (!canvas || !video) return

    const ctx = canvas.getContext("2d")
    if (!ctx) return

    // 비디오 프레임을 캔버스에 그리기
    ctx.drawImage(video, 0, 0, canvas.width, canvas.height)

    // 현재 그리고 있는 선 표시
    if (currentLine) {
      ctx.strokeStyle = "#ff0000" // 빨간색 선
      ctx.lineWidth = 3
      ctx.beginPath()
      ctx.moveTo(currentLine.x1, currentLine.y1)
      ctx.lineTo(currentLine.x2, currentLine.y2)
      ctx.stroke()

      // 시작점과 끝점에 원 표시
      ctx.fillStyle = "#ff0000"
      ctx.beginPath()
      ctx.arc(currentLine.x1, currentLine.y1, 5, 0, 2 * Math.PI)
      ctx.fill()
      ctx.beginPath()
      ctx.arc(currentLine.x2, currentLine.y2, 5, 0, 2 * Math.PI)
      ctx.fill()
    }
  }, [currentLine])

  // 애니메이션 프레임으로 지속적으로 그리기
  useEffect(() => {
    if (!isOpen) return

    const animate = () => {
      drawFrame()
      requestAnimationFrame(animate)
    }

    const animationId = requestAnimationFrame(animate)
    return () => cancelAnimationFrame(animationId)
  }, [isOpen, drawFrame])

  // 마우스 다운 이벤트 (선 그리기 시작)
  const handleMouseDown = (e: React.MouseEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current
    if (!canvas) return

    const rect = canvas.getBoundingClientRect()
    const x = e.clientX - rect.left
    const y = e.clientY - rect.top

    setStartPoint({ x, y })
    setIsDrawing(true)
  }

  // 마우스 이동 이벤트 (선 그리기 중)
  const handleMouseMove = (e: React.MouseEvent<HTMLCanvasElement>) => {
    if (!isDrawing || !startPoint) return

    const canvas = canvasRef.current
    if (!canvas) return

    const rect = canvas.getBoundingClientRect()
    const x = e.clientX - rect.left
    const y = e.clientY - rect.top

    // 현재 선 좌표 업데이트
    setCurrentLine({
      x1: startPoint.x,
      y1: startPoint.y,
      x2: x,
      y2: y,
    })
  }

  // 마우스 업 이벤트 (선 그리기 완료)
  const handleMouseUp = () => {
    if (!isDrawing || !currentLine) return

    setIsDrawing(false)
    setStartPoint(null)

    // 기준선 좌표를 부모 컴포넌트로 전달
    onLineDrawn(currentLine)
  }

  // 선 초기화
  const handleReset = () => {
    setCurrentLine(null)
    setStartPoint(null)
    setIsDrawing(false)
  }

  if (!isOpen) return null

  return (
    <div className="fixed inset-0 bg-black bg-opacity-75 flex items-center justify-center z-50">
      <div className="bg-white rounded-lg p-6 max-w-6xl w-full mx-4">
        {/* 모달 헤더 */}
        <div className="flex justify-between items-center mb-4">
          <h2 className="text-2xl font-semibold">기준선 설정</h2>
          <button onClick={onClose} className="text-gray-500 hover:text-gray-700 text-2xl">
            ×
          </button>
        </div>

        {/* 비디오 및 캔버스 영역 */}
        <div className="relative mb-4">
          {/* 숨겨진 비디오 엘리먼트 (캔버스 그리기용) */}
          <video ref={videoRef} src={rtspUrl} className="hidden" autoPlay muted playsInline />

          {/* 그리기용 캔버스 */}
          <canvas
            ref={canvasRef}
            width={1920}
            height={1080}
            className="w-full border border-gray-300 cursor-crosshair"
            onMouseDown={handleMouseDown}
            onMouseMove={handleMouseMove}
            onMouseUp={handleMouseUp}
          />
        </div>

        {/* 안내 메시지 */}
        <div className="mb-4 p-3 bg-blue-50 border border-blue-200 rounded">
          <p className="text-blue-800">마우스를 드래그하여 차량과 보행자의 방향을 판별할 기준선을 그어주세요.</p>
        </div>

        {/* 버튼 영역 */}
        <div className="flex justify-end gap-3">
          <button onClick={handleReset} className="px-4 py-2 bg-gray-500 text-white rounded hover:bg-gray-600">
            초기화
          </button>
          <button onClick={onClose} className="px-4 py-2 bg-blue-500 text-white rounded hover:bg-blue-600">
            완료
          </button>
        </div>
      </div>
    </div>
  )
}
