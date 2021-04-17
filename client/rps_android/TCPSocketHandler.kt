package ca.bcit.rps_app

import java.net.Socket
import java.io.DataInputStream
import java.io.DataOutputStream
import java.lang.Thread.sleep
import java.nio.charset.Charset
import kotlin.concurrent.thread
import kotlin.system.exitProcess


class TCPSocketHandler(socket: Socket? = null) {

    private val serverSocket: Socket? = socket
    private val dIn: DataInputStream =
        DataInputStream(socket?.getInputStream())
    private val dOut: DataOutputStream =
        DataOutputStream(socket?.getOutputStream())
    private var running: Boolean = false
    private var uid: ByteArray = byteArrayOf(0,0,0,0)
    private val byteArr: ByteArray = byteArrayOf(0,0,0)
    private val initByteArr: ByteArray = byteArrayOf(0,0,0,0,0,0,0)
    var finalResponseArr: ByteArray = byteArrayOf(0,0,0,0,0)

    // 7 , 3 , 3 , 5
    var packet: Packet = Packet(null, null)
    get() = field
    set(value){
        field = value
    }
    fun run() {
        packet.setSendBytes(byteArrayOf(0,0,0,0,1,1,2,1,2))
        dOut.write(packet.getSendBytes())
        dIn.read(initByteArr)
        packet.setUid(initByteArr.takeLast(4).toByteArray())
        println(packet.toString())
        dIn.read(byteArrayOf(0,0,0))

    }

    fun receiveThenSetPacket(byteArr: ByteArray) {
        dIn.read(byteArr)
        packet.setRecvBytes(byteArr)

    }

    fun write(message: String) {
        dOut.write((message + '\n').toByteArray(Charset.defaultCharset()))
    }

    fun make_move(move: Int) {
        var data: ByteArray = uid + byteArrayOf(4, 1, 1, move.toByte())
        packet.setSendBytes(data)
        thread { dOut.write(data) }
        dIn.read(byteArrayOf(0,0,0))

    }

    fun getStatusCode(): Int {
        return byteArr[0].toInt()
    }
    fun getConfirmationCode(): Int {
        return finalResponseArr[1].toInt()
    }
    fun shutdown() {
        running = false
        serverSocket?.close()
        println("${serverSocket?.inetAddress?.hostAddress} closed the connection")
    }

    fun receiveFinal(): ByteArray {
        var byteArr = byteArrayOf(0,0,0,0,0)
        dIn.read(byteArr)
        return byteArr
    }
    fun printByteArrayToString(byteArr: ByteArray) {
        for (b in byteArr) {
            print(b.toInt().toString() + " ")
        }
    }
}