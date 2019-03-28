// Set LABEL variable if empty or not declared
try{ if(LABEL.trim() == "") {throw new Exception();} }catch(Exception e){LABEL="onnx && ci"}; echo "${LABEL}"
try{ if(DOCKER_REGISTRY.trim() == "") {throw new Exception();} }catch(Exception e){DOCKER_REGISTRY="amr-registry.caas.intel.com"}; echo "${DOCKER_REGISTRY}"
// CI settings and constants
PROJECT_NAME = "ngraph_cpp"
CI_ROOT = "ngraph/.ci/onnx/jenkins"
DOCKER_CONTAINER_NAME = "jenkins_ngraph-onnx_ci"
NGRAPH_GIT_ADDRESS = "https://github.com/NervanaSystems/ngraph.git"
JENKINS_GITHUB_CREDENTIAL_ID = "7157091e-bc04-42f0-99fd-dc4da2922a55"

def cloneRepository(String jenkins_github_credential_id, String ngraph_git_address) {
    stage('Clone Repo') {
        dir ("ngraph") {
                checkout([$class: 'GitSCM',
                    branches: [[name: "$CHANGE_BRANCH"]],
                    doGenerateSubmoduleConfigurations: false, extensions: [[$class: 'CloneOption', timeout: 30]], submoduleCfg: [],
                    userRemoteConfigs: [[credentialsId: "${jenkins_github_credential_id}",
                    url: "${ngraph_git_address}"]]])
        }
    }
}

def buildImage(configurationMaps) {
    Closure buildMethod = { configMap ->
        sh """
            ${CI_ROOT}/utils/docker.sh build \
                                --name=${configMap["projectName"]} \
                                --version=${configMap["name"]} \
                                --dockerfile_path=${configMap["dockerfilePath"]} || return 1
        """
    }
    UTILS.createStage("Build_image", buildMethod, configurationMaps)
}

def pushImage(configurationMaps) {
    Closure pushMethod = { configMap ->
        UTILS.propagateStatus("Build_image", configMap["name"])
        sh """
            ${CI_ROOT}/utils/docker.sh push \
                                --docker_registry=${DOCKER_REGISTRY} \
                                --name=${configMap["projectName"]} \
                                --version=${configMap["name"]} || return 1
        """
    }
    UTILS.createStage("Push_image", pushMethod, configurationMaps)
}

def cleanup(configurationMaps) {
    Closure cleanupMethod = { configMap ->
        sh """
            rm -rf ${WORKSPACE}/${BUILD_NUMBER}
        """
    }
    UTILS.createStage("Cleanup", cleanupMethod, configurationMaps)
}

def main(String label, String projectName, String projectRoot, String dockerContainerName, String jenkins_github_credential_id, String ngraph_git_address) {
    node(label) {
        timeout(activity: true, time: 15) {
            WORKDIR = "${WORKSPACE}/${BUILD_NUMBER}"
            def configurationMaps;
            try {
                dir ("${WORKDIR}") {
                    cloneRepository(jenkins_github_credential_id, ngraph_git_address)
                    // Load CI API
                    UTILS = load "${CI_ROOT}/utils/utils.groovy"
                    result = 'SUCCESS'
                    // Create configuration maps
                    configurationMaps = UTILS.getDockerEnvList(projectName, dockerContainerName, projectRoot)
                    // Build and push base images
                    buildImage(configurationMaps)
                    pushImage(configurationMaps)
                }
            }
            catch(e) {
                // Set result to ABORTED if exception contains exit code of a process interrupted by SIGTERM
                if ("$e".contains("143")) {
                    currentBuild.result = "ABORTED"
                } else {
                    currentBuild.result = "FAILURE"
                }
            }
            finally {
                cleanup(configurationMaps)
            }
        }
    }
}

main(LABEL, PROJECT_NAME, CI_ROOT, DOCKER_CONTAINER_NAME, JENKINS_GITHUB_CREDENTIAL_ID, NGRAPH_GIT_ADDRESS)
