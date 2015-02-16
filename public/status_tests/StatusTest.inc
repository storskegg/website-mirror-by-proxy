<?php

abstract class StatusTest
{

    protected $messages = array();

    private $passed = false;
    
    public function __construct() {
        $this->passed = $this->run();
    }
    
    public function __toString() {
        $str = get_class($this) . ' ';
        $str .= $this->passed ? 'passed' : 'did NOT pass';
        $str .= ":\n";
        $str .=  print_r($this->getMessages(), true);
        return $str;
    }

    public function getMessages()
    {
        return $this->messages;
    }

    public function passed()
    {
        return $this->passed;
    }
    
    protected abstract function run();
}