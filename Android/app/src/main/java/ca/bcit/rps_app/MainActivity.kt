package ca.bcit.rps_app

import android.graphics.Color
import android.graphics.Color.green
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import ca.bcit.rps_app.databinding.ActivityMainBinding
import java.lang.Thread.sleep
import java.net.Socket
import kotlin.concurrent.thread
import kotlinx.coroutines.*

val STRING_ROCK:      String = "ROCK\n"
val STRING_PAPER:     String = "PAPER\n"
val STRING_SCISSORS:  String = "SCISSORS\n"

val rpsMap = mapOf(1 to "ROCK", 2 to "PAPER", 3 to "SCISSORS")

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding
    private var socketHandler: TCPSocketHandler = TCPSocketHandler()

    override fun onCreate(savedInstanceState: Bundle?) {
        var gameText: TextView = TextView(this)
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        val view = binding.root
        setContentView(view)
        binding.btnConnect.setOnClickListener{
            val ipAddress = binding.editTextIPAddress.text
            val port = binding.editTextPort.text
            println("HELLO $ipAddress $port")
            thread {
                try {
                    val socket = Socket(ipAddress.toString(), Integer.parseInt(port.toString()))
                    println("Connected to ${socket.inetAddress.hostAddress}")
                    socketHandler = TCPSocketHandler(socket)
                    thread{ socketHandler.run() }
                } catch (e: Exception) {
                    println(e)
                }

            }
            binding.editTextIPAddress.isEnabled = false
            binding.editTextPort.isEnabled = false
            binding.btnConnect.isEnabled = false
            binding.btnRock.isEnabled = true
            binding.btnPaper.isEnabled = true
            binding.btnScissors.isEnabled = true
            gameText.text = "Welcome Player!"
        }
        binding.btnRock.setOnClickListener{
            println(STRING_ROCK)
            binding.btnRock.setTextColor(Color.parseColor("#FF0000"))
            try {
                thread { socketHandler.make_move(1) }
                socketHandler.receiveThenSetPacket()

            } catch (e:java.lang.Exception){println(e)}
            finally {
                disableButtons()
                while(socketHandler.getConfirmationCode() != 3);
                GlobalScope.launch {
                    delay(1000)
                    gameText.text = getWinnerText() }

                socketHandler.shutdown()
            }
        }
        binding.btnPaper.setOnClickListener{
            println(STRING_PAPER)
            binding.btnPaper.setTextColor(Color.parseColor("#FF0000"))
            try {
                thread { socketHandler.make_move(2) }
                socketHandler.receiveThenSetPacket()
            } catch (e:java.lang.Exception){println(e)}
            finally {
                disableButtons()
                while(socketHandler.getConfirmationCode() != 3);
                GlobalScope.launch {
                    delay(1000)
                    gameText.text = getWinnerText() }

                socketHandler.shutdown()
            }
        }
        binding.btnScissors.setOnClickListener{
            println(STRING_SCISSORS)
            binding.btnScissors.setTextColor(Color.parseColor("#FF0000"))
            try {
                thread { socketHandler.make_move(3) }
                socketHandler.receiveThenSetPacket()
            } catch (e:java.lang.Exception){println(e)}
            finally {
                disableButtons()
                while(socketHandler.getConfirmationCode() != 3);
                GlobalScope.launch {
                    delay(1000)
                    gameText.text = getWinnerText() }

                socketHandler.shutdown()
            }

        }
        view.addView(gameText)

    }

    fun disableButtons() {
        binding.btnRock.isEnabled = false
        binding.btnPaper.isEnabled = false
        binding.btnScissors.isEnabled = false

    }

    fun getWinnerText(): String {
        if (socketHandler.getConfirmationCode() == 3) {
            var result = socketHandler.packet.statusUpdate?.get(3)?.toInt()
            var gameResult: String?
            if (result == 1) { gameResult = "YOU WON"} else if (result == 2) { gameResult = "YOU LOST" } else { gameResult = "TIED GAME" }
            val otherPlayerMove = socketHandler.packet.statusUpdate?.get(4)?.toInt()
            println("OTHER PLAY MOVE = $otherPlayerMove")
            return "$gameResult they played ${rpsMap[otherPlayerMove]}! "
        }
        return "Waiting..!"
    }



}