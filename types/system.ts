// 시스템에서 사용하는 타입 정의
export interface LogData {
  id: string // 로그 고유 ID
  timestamp: string // 로그 생성 시간
  date: string // 날짜 (YYYY-MM-DD)
  time: string // 시간 (HH:MM:SS)
  imageUrl: string // 캡처된 이미지 URL
  detectionType: "person" | "vehicle" // 감지된 객체 타입
  direction: "incoming" | "outgoing" // 이동 방향
}

export interface LineCoordinate {
  x1: number // 시작점 X 좌표
  y1: number // 시작점 Y 좌표
  x2: number // 끝점 X 좌표
  y2: number // 끝점 Y 좌표
}

export interface ServerConfig {
  rtspUrl: string // RTSP 스트림 URL
  tcpHost: string // TCP 서버 호스트
  tcpPort: number // TCP 서버 포트
}
