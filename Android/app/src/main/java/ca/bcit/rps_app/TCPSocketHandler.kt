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
    private val byteArr: ByteArray = byteArrayOf(0,0,0,0,0,0,0)

    // 7 , 3 , 3 , 5
    var packet: Packet = Packet(null, null)
    get() = field
    set(value){
        field = value
    }
    fun run() {
        packet.setSendBytes(byteArrayOf(0,0,0,0,1,1,2,1,2))
        dOut.write(packet.getSendBytes())
        receiveThenSetPacket()
        packet.setUid()

        running = true
        if (getStatusCode() in 40..59) {
            shutdown()
            exitProcess(1)
        }
        println("BEEFOREEEE LOOOPP")

        while (running) {
            println(packet.toString())
            try {
                if (getStatusCode() in 40..59) {
                    println("Some error with status. Code: ${getStatusCode()}")
                    shutdown()
                    continue
                } else if (getStatusCode() == 20) {
                    println(packet.toString())
                    packet.setStatusUpdate()
                    receiveThenSetPacket()
                    if (getConfirmationCode() == 3) {
                        printByteArrayToString(byteArr)
                        packet.setRecvBytes(byteArr)
                        shutdown()
                        println("GAME OVER")
                        return
                    }
                    continue
                }
                receiveThenSetPacket()

            } catch (e: Exception) {
                println(e)
                shutdown()
            }
        }
    }

    fun receiveThenSetPacket() {
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
    }

    fun getStatusCode(): Int {
        return byteArr[0].toInt()
    }
    fun getConfirmationCode(): Int {
        return byteArr[1].toInt()
    }
    fun shutdown() {
        running = false
        serverSocket?.close()
        println("${serverSocket?.inetAddress?.hostAddress} closed the connection")
    }

    fun printByteArrayToString(byteArr: ByteArray) {
        for (b in byteArr) {
            print(b.toInt().toString() + " ")
        }
    }
}