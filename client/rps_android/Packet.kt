package ca.bcit.rps_app

class Packet {
    private var recvBytes: ByteArray? = null
    private var sendBytes: ByteArray? = null
    private var uid: ByteArray? = null
    var statusUpdate: ByteArray? = null
    var playerNum: Int? = null
        get() = field
        set(value) {
            field = value
        }

    constructor(sendBytes: ByteArray?, recvBytes: ByteArray?) {
        this.sendBytes = sendBytes
        this.recvBytes = recvBytes
    }

    fun getRecvBytes(): ByteArray? {
        return recvBytes
    }
    fun setRecvBytes(bytes: ByteArray?) {
        if (bytes != null) {
            recvBytes = bytes
        }
    }
    fun getSendBytes(): ByteArray? {
        return sendBytes
    }
    fun setSendBytes(bytes: ByteArray?) {
        if (bytes != null) {
            sendBytes = bytes
        }
    }
    fun setUid(byteArr: ByteArray) {
        uid = byteArr
    }
    fun setStatusUpdate() {
        statusUpdate = recvBytes?.sliceArray(0..4)
    }

    override fun toString(): String {
        return "Packet(recvBytes=${recvBytes?.contentToString()}," +
                " sendBytes=${sendBytes?.contentToString()}, " +
                "uid=${uid?.contentToString()}," +
                " statusUpdate=${statusUpdate?.contentToString()})"
    }


}

fun main() {
}