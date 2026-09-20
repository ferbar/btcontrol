// import org.gradle.kotlin.dsl.java
// import java.io.ByteArrayOutputStream
// import kotlin.toString

/*
fun gitCommitHash(): String {
    return providers.exec {
        commandLine("git", "rev-parse", "--short", "HEAD")
    }.standardOutput.asText.get().trim()
}*/

fun gitCommitCount(): Int {
    return providers.exec {
        commandLine("git", "rev-list", "--count", "HEAD")
    }.standardOutput.asText.get().trim().toInt()
}

fun isDirty(): Boolean {
    return providers.exec {
        commandLine("git", "status", "--porcelain")
    }.standardOutput.asText.get().isNotBlank()
}

plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "org.ferbar.btcontrol"
    compileSdk {
        version = release(37)
    }

    defaultConfig {
        applicationId = "org.ferbar.btcontrol"
        /*
        9 = Android 2.3
       14 = 4.0
       16 = 4.1
       34 = Android 14
         */
        minSdk = 16
        targetSdk = 34
        versionCode = gitCommitCount()
        versionName = "1.${gitCommitCount()}" + if (isDirty()) "-dirty" else ""


        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            optimization {
                enable = false
            }
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    packaging {
        resources {
            excludes += "META-INF/DEPENDENCIES"
        }
    }
}

dependencies {
    implementation(libs.activity.ktx)
    implementation(libs.appcompat)
    implementation(libs.constraintlayout)
    implementation(libs.material)
    testImplementation(libs.junit)
    androidTestImplementation(libs.espresso.core)
    androidTestImplementation(libs.ext.junit)
    // für JmDNS
    implementation(project(":jmdns-patched"))
    // war bis API28 beim android dabei, TODO: durch HttpURLConnection (Android) ersetzen
    //implementation("org.apache.httpcomponents:httpclient:4.5.14")
}

