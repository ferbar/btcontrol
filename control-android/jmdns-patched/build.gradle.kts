plugins {
    // eigentlich isses ja java, aber dann geht das android logging nicht
    // id("java-library")
    id("com.android.library")
}

java {
    sourceCompatibility = JavaVersion.VERSION_1_8
    targetCompatibility = JavaVersion.VERSION_1_8
}

android {
    namespace = "javax.jmdns"
    compileSdk = 34

    defaultConfig {
        minSdk = 16
    }

    sourceSets {
        getByName("main") {
            java.setSrcDirs(listOf("jmdns/src/main/java"))
        }
    }
}
/* für java modul:
sourceSets {
    main {
        java {
            // setSrcDirs(listOf("jmdns/src/main/java/javax/jmdns/"))
            setSrcDirs(listOf("jmdns/src/main/java"))
        }
    }
}
*/

/*
repositories {
    mavenCentral()
}
 */

dependencies {
    implementation("org.slf4j:slf4j-api:1.7.21")
}