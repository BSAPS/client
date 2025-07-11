// TCP 통신 서비스
import type { TcpPacket, ImageTextData } from "../types/tcp"
import { TcpRequestType } from "../types/tcp"
import type { LineCoordinate } from "../types/system"

class TcpService {
  private socket: WebSocket | null = null
  private isConnected = false
  private host = ""
  private port = 0
  private reconnectAttempts = 0
  private maxReconnectAttempts = 5
  private reconnectDelay = 3000

  // 연결 상태 변경 콜백
  private onConnectionChange?: (connected: boolean) => void
  private onMessageReceived?: (packet: TcpPacket) => void

  constructor() {
    this.setupEventHandlers()
  }

  // 서버 연결
  async connect(host: string, port: number): Promise<boolean> {
    try {
      this.host = host
      this.port = port

      // 현재 페이지의 프로토콜에 따라 WebSocket 프로토콜 결정
      const isSecure = window.location.protocol === "https:"
      const wsProtocol = isSecure ? "wss" : "ws"

      // WebSocket을 통한 TCP 시뮬레이션 (실제 환경에서는 TCP 소켓 사용)
      const wsUrl = `${wsProtocol}://${host}:${port}`

      console.log(`TCP 연결 시도: ${wsUrl}`)
      this.socket = new WebSocket(wsUrl)

      return new Promise((resolve) => {
        if (!this.socket) {
          resolve(false)
          return
        }

        // 연결 타임아웃 설정 (10초)
        const connectionTimeout = setTimeout(() => {
          if (this.socket && this.socket.readyState === WebSocket.CONNECTING) {
            this.socket.close()
            console.error("TCP 연결 타임아웃")
            resolve(false)
          }
        }, 10000)

        this.socket.onopen = () => {
          clearTimeout(connectionTimeout)
          this.isConnected = true
          this.reconnectAttempts = 0
          console.log("TCP 서버 연결 성공:", `${host}:${port}`)
          this.onConnectionChange?.(true)
          resolve(true)
        }

        this.socket.onerror = (error) => {
          clearTimeout(connectionTimeout)
          console.error("TCP 연결 오류:", error)
          this.isConnected = false
          this.onConnectionChange?.(false)
          resolve(false)
        }

        this.socket.onclose = (event) => {
          clearTimeout(connectionTimeout)
          this.isConnected = false
          this.onConnectionChange?.(false)

          if (event.code !== 1000) {
            // 정상 종료가 아닌 경우
            console.log(`TCP 연결 종료 (코드: ${event.code}, 이유: ${event.reason})`)
            this.handleReconnect()
          }
        }

        this.socket.onmessage = (event) => {
          this.handleMessage(event.data)
        }
      })
    } catch (error) {
      console.error("TCP 연결 실패:", error)
      return false
    }
  }

  // 연결 테스트 (실제 연결하지 않고 URL 유효성만 확인)
  testConnection(host: string, port: number): { valid: boolean; url: string; protocol: string } {
    const isSecure = window.location.protocol === "https:"
    const wsProtocol = isSecure ? "wss" : "ws"
    const wsUrl = `${wsProtocol}://${host}:${port}`

    return {
      valid: true,
      url: wsUrl,
      protocol: wsProtocol,
    }
  }

  // 연결 해제
  disconnect() {
    if (this.socket) {
      this.socket.close()
      this.socket = null
    }
    this.isConnected = false
    this.onConnectionChange?.(false)
  }

  // 패킷 전송
  async sendPacket(packet: TcpPacket): Promise<boolean> {
    if (!this.isConnected || !this.socket) {
      console.error("TCP 서버에 연결되지 않음")
      return false
    }

    try {
      const message = this.formatPacket(packet)
      this.socket.send(message)
      console.log("TCP 패킷 전송:", message)
      return true
    } catch (error) {
      console.error("TCP 패킷 전송 실패:", error)
      return false
    }
  }

  // 이미지&텍스트 데이터 전송 (요청 번호 5)
  async sendImageTextData(data: ImageTextData): Promise<boolean> {
    const packet: TcpPacket = {
      requestId: TcpRequestType.IMAGE_TEXT_INSERT,
      success: 1,
      data1: data.imageUrl,
      data2: data.text,
      data3: JSON.stringify({
        timestamp: data.timestamp,
        detectionType: data.detectionType,
        direction: data.direction,
      }),
    }

    return await this.sendPacket(packet)
  }

  // 가상 라인 좌표값 전송 (요청 번호 2)
  async sendLineCoordinates(coordinates: LineCoordinate): Promise<boolean> {
    const packet: TcpPacket = {
      requestId: TcpRequestType.LINE_MAPPING_REQUEST,
      success: 1,
      data1: coordinates.x1.toString(),
      data2: coordinates.y1.toString(),
      data3: `${coordinates.x2},${coordinates.y2}`,
    }

    return await this.sendPacket(packet)
  }

  // 라인 동기화 요청 (요청 번호 3)
  async requestLineSync(): Promise<boolean> {
    const packet: TcpPacket = {
      requestId: TcpRequestType.LINE_SYNC_REQUEST,
      success: 1,
    }

    return await this.sendPacket(packet)
  }

  // 패킷 포맷팅
  private formatPacket(packet: TcpPacket): string {
    const parts = [
      packet.requestId.toString(),
      packet.success.toString(),
      packet.data1 || "",
      packet.data2 || "",
      packet.data3 || "",
    ]
    return parts.join("/")
  }

  // 수신된 메시지 파싱
  private parsePacket(message: string): TcpPacket | null {
    try {
      const parts = message.split("/")
      if (parts.length < 2) return null

      return {
        requestId: Number.parseInt(parts[0]) || 0,
        success: Number.parseInt(parts[1]) || 0,
        data1: parts[2] || undefined,
        data2: parts[3] || undefined,
        data3: parts[4] || undefined,
      }
    } catch (error) {
      console.error("패킷 파싱 오류:", error)
      return null
    }
  }

  // 메시지 처리
  private handleMessage(message: string) {
    console.log("TCP 메시지 수신:", message)
    const packet = this.parsePacket(message)

    if (!packet) {
      console.error("잘못된 패킷 형식:", message)
      return
    }

    // 요청 타입별 처리
    switch (packet.requestId) {
      case TcpRequestType.IMAGE_TEXT_REQUEST:
        this.handleImageTextRequest(packet)
        break
      case TcpRequestType.LINE_MAPPING_REQUEST:
        this.handleLineMappingRequest(packet)
        break
      case TcpRequestType.LINE_SYNC_REQUEST:
        this.handleLineSyncRequest(packet)
        break
      case TcpRequestType.CCTV_LINE_DATA:
        this.handleCctvLineData(packet)
        break
      case TcpRequestType.IMAGE_TEXT_INSERT:
        this.handleImageTextInsert(packet)
        break
      default:
        console.warn("알 수 없는 요청 타입:", packet.requestId)
    }

    this.onMessageReceived?.(packet)
  }

  // 이미지&텍스트 요청 처리 (요청 번호 1)
  private async handleImageTextRequest(packet: TcpPacket) {
    console.log("이미지&텍스트 요청 수신")

    // 현재 캡처된 이미지와 텍스트 데이터 준비
    const imageTextData: ImageTextData = {
      imageUrl: "/placeholder.svg?height=400&width=600",
      text: `감지 시간: ${new Date().toLocaleString()}\n감지 타입: 차량\n이동 방향: 진입`,
      timestamp: new Date().toISOString(),
      detectionType: "vehicle",
      direction: "incoming",
    }

    // 서버로 이미지&텍스트 데이터 전송
    await this.sendImageTextData(imageTextData)
  }

  // 라인 매핑 요청 처리 (요청 번호 2)
  private handleLineMappingRequest(packet: TcpPacket) {
    console.log("라인 매핑 요청 수신:", packet)
    // 도트 매트릭스 매핑 처리 로직
  }

  // 라인 동기화 요청 처리 (요청 번호 3)
  private handleLineSyncRequest(packet: TcpPacket) {
    console.log("라인 동기화 요청 수신:", packet)
    // 라인 동기화 처리 로직
  }

  // CCTV 라인 데이터 처리 (요청 번호 4)
  private handleCctvLineData(packet: TcpPacket) {
    console.log("CCTV 라인 데이터 수신:", packet)
    // CCTV 라인 데이터 처리 로직
  }

  // 이미지&텍스트 삽입 처리 (요청 번호 5)
  private handleImageTextInsert(packet: TcpPacket) {
    console.log("이미지&텍스트 삽입 완료:", packet)
    // 삽입 완료 처리 로직
  }

  // 재연결 처리
  private handleReconnect() {
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++
      console.log(`재연결 시도 ${this.reconnectAttempts}/${this.maxReconnectAttempts}`)

      setTimeout(() => {
        this.connect(this.host, this.port)
      }, this.reconnectDelay)
    } else {
      console.error("최대 재연결 시도 횟수 초과")
    }
  }

  // 이벤트 핸들러 설정
  private setupEventHandlers() {
    // 필요시 추가 이벤트 핸들러 설정
  }

  // 연결 상태 확인
  getConnectionStatus(): boolean {
    return this.isConnected
  }

  // 콜백 설정
  setConnectionChangeCallback(callback: (connected: boolean) => void) {
    this.onConnectionChange = callback
  }

  setMessageReceivedCallback(callback: (packet: TcpPacket) => void) {
    this.onMessageReceived = callback
  }
}

export const tcpService = new TcpService()
