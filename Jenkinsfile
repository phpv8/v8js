pipeline {
    agent none
    stages {
        stage('BuildAndTest') {
            matrix {
                agent any
                axes {
                    axis {
                        name 'PHPVER'
                        values '7.3.21', '7.4.9'
                    }
                    axis {
                        name 'V8VER'
                        values '7.9', '8.4', '8.6'
                    }
                }
                stages {
                    stage('Build') {
                        steps {
                            echo "Building w/ V8 ${V8VER}, PHP ${PHPVER} as Docker image ${BUILD_TAG}-${V8VER}-${PHPVER}"
                            sh "docker build -f Dockerfile.jenkins --build-arg V8VER=${V8VER} --build-arg PHPVER=${PHPVER} -t ${BUILD_TAG}-${V8VER}-${PHPVER} ."
                        }
                    }
                    stage('Test') {
                        steps {
                            echo "Running test on ${BUILD_TAG}-${V8VER}-${PHPVER}"
                            sh "docker run --rm -t ${BUILD_TAG}-${V8VER}-${PHPVER} make test TESTS='ext/v8js/tests/*.phpt'"
                        }
                    }
                }
            }
        }
    }
}
