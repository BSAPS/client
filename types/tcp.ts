// TCP 통신 관련 타입 정의
export interface TcpPacket {
  requestId: number // 요청 번호 (0-5)
  success: number // 성공:1, 실패:0
  data1?: string // 데이터1
  data2?: string // 데이터2
  data3?: string // 데이터3
}

export interface TcpRequest {
  requestId: number
  data?: any
}

export interface TcpResponse {
  requestId: number
  success: boolean
  data?: any
}

// TCP 요청 타입 정의
export enum TcpRequestType {
  IMAGE_TEXT_REQUEST = 1, // 이미지&텍스트 요청
  LINE_MAPPING_REQUEST = 2, // 가상 라인 좌표값 - 도트 매트릭스 매핑 요청
  LINE_SYNC_REQUEST = 3, // 가상 라인 좌표값 동기화 요청
  CCTV_LINE_DATA = 4, // CCTV의 가상 라인 좌표값 데이터
  IMAGE_TEXT_INSERT = 5, // 이미지&텍스트 삽입
}

export interface ImageTextData {
  imageUrl: string
  text: string
  timestamp: string
  detectionType: "person" | "vehicle"
  direction: "incoming" | "outgoing"
}
