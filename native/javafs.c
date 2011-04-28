#include "javafs.h"
#include "fuse_callback.h"
#include "util.h"

int main(int argc, char *argv[])
{
    int i;
    jfuse_params *params = calloc(1, sizeof(jfuse_params));
    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

    for(i = 0; i < argc; i++)
    {
        char *arg = argv[i];
        
        // Fuse only captures command name, mount point and -f (foreground mode)
        if (i < 2 || !strcmp(arg, "-f"))
          fuse_opt_add_arg(&args, argv[i]);
        
        

        // Suppress all -o arguments so that we can choose to add them if we want them.
        if (!strcmp(arg, "-o")) {}        
        else if (!strncmp(arg, "class=", 6))
            params->filesystemClassName = &(arg[6]);
        else if (!strncmp(arg, "jvm=", 4))
            params->jvmArgv[(params->jvmArgc)++] = &(arg[4]);
        else if (!strncmp(arg, "java=", 5))
            params->javaArgv[(params->javaArgc)++] = &(arg[5]);
        else if (params->javaArgc == 1)
            params->javaArgv[(params->javaArgc)++] = argv[i];
        else if (i>0) { // Don't capture argv[0] - command name
            // Done with special handling so re-add -o if needed.
            if (!strcmp(argv[i - 1], "-o"))
                params->javaArgv[(params->javaArgc)++] = "-o";
            params->javaArgv[(params->javaArgc)++] = argv[i];
        }
    }

    if (params->filesystemClassName == NULL)
    {
        printf("Missing option: class=fuse.FuseFSClassName\n");
        return -1;
    }

    return fuse_main(args.argc, args.argv, &javafs_oper, params);
}
