"use client"

// TCP 디버그 패널 컴포넌트
import { useState } from "react"
import { useTcpService } from "../hooks/useTcpService"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import type { ImageTextData } from "../types/tcp"

export function TcpDebugPanel() {
  const { isConnected, lastMessage, sendImageTextData, sendLineCoordinates, requestLineSync } = useTcpService()

  const [debugLog, setDebugLog] = useState<string[]>([])

  const addLog = (message: string) => {
    setDebugLog((prev) => [...prev.slice(-9), `${new Date().toLocaleTimeString()}: ${message}`])
  }

  // 테스트 이미지&텍스트 데이터 전송
  const handleSendTestImageText = async () => {
    const testData: ImageTextData = {
      imageUrl: "/placeholder.svg?height=400&width=600",
      text: "테스트 감지 데이터",
      timestamp: new Date().toISOString(),
      detectionType: "vehicle",
      direction: "incoming",
    }

    const success = await sendImageTextData(testData)
    addLog(`이미지&텍스트 전송: ${success ? "성공" : "실패"}`)
  }

  // 테스트 라인 좌표 전송
  const handleSendTestLine = async () => {
    const testCoordinates = {
      x1: 100,
      y1: 200,
      x2: 300,
      y2: 400,
    }

    const success = await sendLineCoordinates(testCoordinates)
    addLog(`라인 좌표 전송: ${success ? "성공" : "실패"}`)
  }

  // 라인 동기화 요청
  const handleRequestSync = async () => {
    const success = await requestLineSync()
    addLog(`라인 동기화 요청: ${success ? "성공" : "실패"}`)
  }

  return (
    <Card className="w-full max-w-md">
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          TCP 디버그 패널
          <div className={`w-2 h-2 rounded-full ${isConnected ? "bg-green-500" : "bg-red-500"}`} />
        </CardTitle>
      </CardHeader>
      <CardContent className="space-y-4">
        {/* 테스트 버튼들 */}
        <div className="space-y-2">
          <Button
            onClick={handleSendTestImageText}
            disabled={!isConnected}
            className="w-full bg-transparent"
            variant="outline"
          >
            테스트 이미지&텍스트 전송
          </Button>

          <Button
            onClick={handleSendTestLine}
            disabled={!isConnected}
            className="w-full bg-transparent"
            variant="outline"
          >
            테스트 라인 좌표 전송
          </Button>

          <Button
            onClick={handleRequestSync}
            disabled={!isConnected}
            className="w-full bg-transparent"
            variant="outline"
          >
            라인 동기화 요청
          </Button>
        </div>

        {/* 마지막 수신 메시지 */}
        {lastMessage && (
          <div className="p-3 bg-gray-100 rounded-lg">
            <h4 className="font-medium text-sm mb-2">마지막 수신 메시지:</h4>
            <pre className="text-xs overflow-x-auto">{JSON.stringify(lastMessage, null, 2)}</pre>
          </div>
        )}

        {/* 디버그 로그 */}
        <div className="p-3 bg-gray-50 rounded-lg">
          <h4 className="font-medium text-sm mb-2">디버그 로그:</h4>
          <div className="text-xs space-y-1 max-h-32 overflow-y-auto">
            {debugLog.length === 0 ? (
              <p className="text-gray-500">로그가 없습니다</p>
            ) : (
              debugLog.map((log, index) => (
                <div key={index} className="font-mono">
                  {log}
                </div>
              ))
            )}
          </div>
        </div>
      </CardContent>
    </Card>
  )
}
