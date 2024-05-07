# Task Orchestrator Service

A task orchestrator service implemented in C within Operating Systems environment in 2023/2024 academic year.

- [Description and Requirements](project.pdf)
- [Report](report/report.pdf)
- [Documentation](docs/html/index.html) **TODO display link instead**

## Description

The task orchestrator is a service that allows the asynchronous execution of tasks.

The service is composed of two main components:

- **Server**: responsible for managing the tasks execution or scheduling and transmitting messages to clients.
- **Client**: has the ability to execute tasks and check their status.

## Scheduling Policies

The service supports the following scheduling policies:
- **FCFS**: First Come First Served
- **SJF**: Shortest Job First
- **PES**: Priority Escalation Scheduling

## Compilation

To compile the project, run the following command:

```sh
make
```

The `orchestrator` and `client` executables will be generated in the `bin` directory.

## Usage

### Orchestrator Server Usage

```
Usage: bin/orchestrator <output_dir> <parallel_tasks> [sched_policy]

Options:
    <output_dir>      Directory to store task output and history files
    <parallel_tasks>  Maximum number of tasks running in parallel
    [sched_policy]    Scheduling policy (FCFS, SJF, PES) default: SJF
```

### Client Usage

```
Usage: bin/client <option [args]>

Options:
    execute <est_time|priority> <-u|-p> "<command [args]>"  Execute a command
        est_time  Estimated time of execution, in case scheduling policy is SJF
        priority  Priority of the task, in case scheduling policy is PES
        -u        Unpiped command
        -p        Piped command
        command   Command to execute. Maximum size: 300 bytes
    status                                                  Status of the tasks
    kill                                                    Terminate the server

Note: The estimated time or priority is irrelevant if the scheduling policy is
    FCFS, but it must be provided as an argument.
```

#### Execute Task

```
bin/client execute <est_time|priority> <-u|-p> "<command [args]>"
```

#### Check Tasks Status

Run the following command to check the status of the tasks:

```
bin/client status
```

This command will display all the executing, scheduled, and completed tasks.
For completed tasks the elapsed time is also displayed.

#### Kill Orchestrator Server

Shutdown the orchestrator server, with the following command:

```
bin/client kill
```

This command will send a request to the orchestrator server to shutdown.

This terminates all the scheduled tasks and the server itself.

Executing tasks will still be able to finish, and the respective files will be saved in the output directory, as usual.

## Output Files

The output directory has the given structure:

```
<output_dir>
 ├── history
 ├── task_number
 ├── task1
 │   ├── out
 │   ├── err
 │   └── time
 ├── task2
 │   ├── out
 │   ├── err
 │   └── time
 │
...
 │
 └── taskN
     ├── out
     ├── err
     └── time
```

## Testing

You can find a selection of test cases in the `tests` directory.

This tests require the `orchestrator` to be running.

Run then from the project root directory, so the scripts can find the client executable.

## Authors

- Flávia Araújo - [@flaviaraujo](https://github.com/flaviaraujo)
- Miguel Carvalho - [@migueltc13](https://github.com/migueltc13)
