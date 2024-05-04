# Task Orchestrator

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

```sh
bin/orchestrator <output_dir> <parallel_tasks> [sched_policy]
```

- `output_dir`: path to the directory where the output directories and files will be saved.
- `parallel_tasks`: number of tasks that can be executed in parallel.
- `sched_policy`: scheduling policy to be used. If not provided, the default policy is SJF.

### Client Usage

#### Execute Task

```sh
bin/client execute <estimated_time|priority> <-u|-p> "<command>"
```

The estimated time or priority must be provided,
depending on the scheduling policy used in the orchestrator server.

- `estimated_time`: estimated time for the task to complete.
- `priority`: priority of the task. The higher the value, the higher the priority.

The `-u` flag is used to indicate that the command being executed is a single command.
For executing pipelines, the `-p` flag must be used.

- `command`: command to be executed, with its arguments, if any. Hard-coded maximum size of 300 characters.

#### Check Tasks Status

Run the following command to check the status of the tasks:

```sh
bin/client status
```

This command will display all the executing, scheduled, and completed tasks.
For completed tasks the elapsed time is also displayed.

#### Kill Orchestrator Server

Shutdown the orchestrator server, with the following command:

```sh
bin/client kill
```

This command will send a message to the orchestrator server to shutdown.

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
