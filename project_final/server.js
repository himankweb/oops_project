const express = require('express');
const path = require('path');
const { spawn } = require('child_process');

const app = express();
const port = 3000;

app.set('view engine', 'ejs');

app.use(express.json({ limit: '1mb' }));
app.use(express.static(path.join(__dirname, 'public')));

// Home page
app.get('/', (req, res) => {
    const sample = `# Welcome to Markdown Studio

Write your Markdown here.

## Features

- **Bold text**
- *Italic text*
- Headings and paragraphs
- Links and code

[Visit Google](https://google.com)

> Start writing your Markdown!`;

    res.render('index', { sample: sample });
});

// Convert Markdown using the C++ program
app.post('/convert', (req, res) => {
    const markdown = req.body.markdown || '';

    const program = path.join(__dirname, 'cpp', 'md2html.exe');
    const converter = spawn(program, []);

    let html = '';
    let error = '';
    let failed = false;

    converter.stdout.on('data', (data) => {
        html += data.toString();
    });

    converter.stderr.on('data', (data) => {
        error += data.toString();
    });

    converter.on('error', () => {
        failed = true;

        res.status(500).json({
            error: 'C++ program not found. Compile cpp/main.cpp first.'
        });
    });

    converter.on('close', (code) => {
        if (failed) return;

        if (code === 0) {
            res.json({ html: html });
        } else {
            res.status(500).json({
                error: error || 'Markdown conversion failed.'
            });
        }
    });

    converter.stdin.end(markdown);
});

// Start server
app.listen(port, '0.0.0.0', () => {
    console.log(`Markdown Studio running on port ${port}`);
});