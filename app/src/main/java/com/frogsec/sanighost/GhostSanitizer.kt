package com.frogsec.sanighost

import java.io.File
import java.io.RandomAccessFile
import java.security.SecureRandom
import android.database.sqlite.SQLiteDatabase

/**
 * Native Kotlin SaniGhost Core Engine
 * Performs multi-pass shredding, fsync hardware buffer flushing, and SQLite sanitization.
 */
class GhostSanitizer(
    val passes: Int = 3,
    val dryRun: Boolean = false,
    val onProgress: ((status: String, path: String, bytes: Long) -> Unit)? = null
) {
    data class SanitizationStats(
        var filesShredded: Int = 0,
        var bytesOverwritten: Long = 0,
        var dbsSanitized: Int = 0,
        var errors: Int = 0,
        var durationMs: Long = 0
    )

    fun shredFile(file: File): Boolean {
        if (!file.exists()) return false

        val length = file.length()
        if (dryRun) {
            onProgress?.invoke("WOULD_SHRED", file.absolutePath, length)
            return true
        }

        try {
            if (length > 0) {
                RandomAccessFile(file, "rwd").use { raf ->
                    val channel = raf.channel
                    val random = SecureRandom()
                    val buffer = ByteArray(65536)

                    for (pass in 1..passes) {
                        raf.seek(0)
                        var written = 0L

                        while (written < length) {
                            val chunkSize = minOf(buffer.size.toLong(), length - written).toInt()
                            when {
                                pass == 1 -> buffer.fill(0x00.toByte())
                                pass == 2 && passes >= 3 -> buffer.fill(0xFF.toByte())
                                else -> random.nextBytes(buffer)
                            }
                            raf.write(buffer, 0, chunkSize)
                            written += chunkSize
                        }
                        channel.force(true)
                    }
                }
            }
            val deleted = file.delete()
            if (deleted) {
                onProgress?.invoke("SHREDDED", file.absolutePath, length)
            }
            return deleted
        } catch (e: Exception) {
            onProgress?.invoke("ERROR: ${e.message}", file.absolutePath, 0)
            return false
        }
    }

    fun sanitizeDatabase(dbFile: File): Boolean {
        if (!dbFile.exists()) return false

        if (!dryRun) {
            try {
                SQLiteDatabase.openDatabase(dbFile.absolutePath, null, SQLiteDatabase.OPEN_READWRITE).use { db ->
                    val cursor = db.rawQuery("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';", null)
                    while (cursor.moveToNext()) {
                        val tableName = cursor.getString(0)
                        db.execSQL("DELETE FROM \"$tableName\"")
                    }
                    cursor.close()
                    db.execSQL("VACUUM")
                }
            } catch (e: Exception) {
                onProgress?.invoke("DB_WARNING: ${e.message}", dbFile.absolutePath, 0)
            }
        }
        return shredFile(dbFile)
    }

    fun purge(targetDirs: List<File>, targetFiles: List<File>, targetDbs: List<File>): SanitizationStats {
        val stats = SanitizationStats()
        val start = System.currentTimeMillis()

        targetFiles.forEach { if (shredFile(it)) stats.filesShredded++ else stats.errors++ }
        targetDbs.forEach { if (sanitizeDatabase(it)) stats.dbsSanitized++ else stats.errors++ }

        targetDirs.forEach { dir ->
            if (dir.exists()) {
                dir.walkBottomUp().forEach { file ->
                    if (file.isFile) {
                        if (shredFile(file)) stats.filesShredded++ else stats.errors++
                    } else if (!dryRun) {
                        file.delete()
                    }
                }
            }
        }

        stats.durationMs = System.currentTimeMillis() - start
        return stats
    }
}
