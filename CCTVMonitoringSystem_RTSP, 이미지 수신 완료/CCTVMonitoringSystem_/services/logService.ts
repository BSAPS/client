// 로그 데이터 관리 서비스
import type { LogData } from "../types/system"

class LogService {
  private logs: LogData[] = [] // 로그 데이터 저장소

  // 서버에서 로그 데이터 가져오기
  async fetchLogs(): Promise<LogData[]> {
    try {
      // 실제 구현에서는 서버 API 호출
      // const response = await fetch('/api/logs');
      // const logs = await response.json();

      // 시뮬레이션 데이터
      const simulatedLogs: LogData[] = [
        {
          id: "1",
          timestamp: "2024-01-15T10:30:00Z",
          date: "2024-01-15",
          time: "10:30:00",
          imageUrl: "/placeholder.svg?height=400&width=600",
          detectionType: "vehicle",
          direction: "incoming",
        },
        {
          id: "2",
          timestamp: "2024-01-15T11:15:00Z",
          date: "2024-01-15",
          time: "11:15:00",
          imageUrl: "/placeholder.svg?height=400&width=600",
          detectionType: "person",
          direction: "outgoing",
        },
      ]

      this.logs = simulatedLogs
      return simulatedLogs
    } catch (error) {
      console.error("로그 데이터 가져오기 실패:", error)
      return []
    }
  }

  // 날짜별 로그 필터링
  getLogsByDate(date: string): LogData[] {
    return this.logs.filter((log) => log.date === date)
  }

  // 시간별 로그 필터링
  getLogsByTime(time: string): LogData[] {
    return this.logs.filter((log) => log.time.startsWith(time))
  }

  // 특정 로그 가져오기
  getLogById(id: string): LogData | undefined {
    return this.logs.find((log) => log.id === id)
  }
}

export const logService = new LogService()
