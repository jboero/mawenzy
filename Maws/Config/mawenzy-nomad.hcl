# Nomad job for running Mawd
# John Boero
job "mawd" {
    datacenters = ["dc1"]

    type = "service"

    task "server" {
        driver = "raw_exec"
        user    = "jboero"
        
        env {
            "MAWS_PLAT" = "0"
            "MAWS_DEV"  = "0"
            "MAWS_SOCK" = "/tmp/mawd.sock"
        }
        
        # GPU Cgroups may be arriving shortly...
        # https://lists.freedesktop.org/archives/intel-gfx/2019-May/197206.html
        resources {
            cpu     = 4000
            memory  = 4096
        }

        config {
            command = "/usr/bin/mawd"
            args    = ["-s", "-f", "-o", "big_writes,max_write=262144,max_read=262144", "/mnt/gpu"]
        }
    }
}
