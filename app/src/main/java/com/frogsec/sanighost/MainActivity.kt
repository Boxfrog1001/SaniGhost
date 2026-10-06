package com.frogsec.sanighost

import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.widget.Button
import android.widget.CheckBox
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import java.io.File
import kotlin.concurrent.thread

class MainActivity : AppCompatActivity() {

    private lateinit var statusText: TextView
    private lateinit var btnPurge: Button
    private lateinit var cbDryRun: CheckBox

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        statusText = findViewById(R.id.statusText)
        btnPurge = findViewById(R.id.btnPurge)
        cbDryRun = findViewById(R.id.cbDryRun)

        checkPermissions()

        btnPurge.setOnClickListener {
            runSanitizer()
        }
    }

    private fun checkPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                val intent = Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                startActivity(intent)
            }
        }
    }

    private fun runSanitizer() {
        val isDryRun = cbDryRun.isChecked
        statusText.text = "Status: Running sanitization..."

        thread {
            val cacheDir = File(cacheDir, "buffers")
            val targetDirs = listOf(cacheDir)

            val sanitizer = GhostSanitizer(passes = 3, dryRun = isDryRun) { status, path, _ ->
                runOnUiThread {
                    statusText.append("\n[$status] $path")
                }
            }

            val stats = sanitizer.purge(targetDirs = targetDirs, targetFiles = emptyList(), targetDbs = emptyList())

            runOnUiThread {
                statusText.text = "Completed in ${stats.durationMs}ms!\nFiles shredded: ${stats.filesShredded}\nErrors: ${stats.errors}"
                Toast.makeText(this, "Sanitization complete!", Toast.LENGTH_SHORT).show()
            }
        }
    }
}
